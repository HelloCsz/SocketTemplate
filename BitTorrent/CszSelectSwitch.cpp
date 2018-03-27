#include <butil/files/file.h> //butil::file
#include <butil/files/file_path.h> //butil::file_path
#include <bthread/bthread.h> //bthread_usleep
#include <butil/time.h> //gettimeofday_s
#include <sys/select.h> //select
#include <sys/socket.h> //getsockopt,setsockopt,recv,send
#include <sys/epoll.h>
#include "CszBitTorrent.h"
#include "../Thread/CszSingletonThread.hpp"
#include "../Sock/CszSocket.h" //RecvTime_us

namespace Csz
{
#ifdef CszTest
    int SelectSwitch::total= 0;
#endif
	//TODO sinnal deal or pselect
	bool SelectSwitch::operator()()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		using Task= Csz::TaskQueue<SelectSwitch::Parameter,THREADNUM>::Type;
		auto thread_pool= SingletonThread<SelectSwitch::Parameter,THREADNUM>::GetInstance();
		auto peer_manager= PeerManager::GetInstance();
		int code;
		const int MAX_EVENTS= 50;
		struct epoll_event ev,events[MAX_EVENTS];	
		//2.select
		while (true)
		{
			auto peer_list= std::move(peer_manager->RetSocketList());
			if (peer_list.empty())
			{
				Csz::ErrMsg("[Select Switch Run]->failed line:%d,can't switch message type,peer list size < 35",__LINE__);
				break;
			}
			auto epollfd= epoll_create(peer_list.size());
			if (-1== epollfd)
			{
				Csz::ErrRet("[Peer Manager Run]->failed line:%d,epoll create,",__LINE__);
				break;
			}
			for (const auto& val : peer_list)
			{
				if (val>= 0)
				{
					//set horizontal mark
					int lowat= 4;
					setsockopt(val,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
					ev.events= EPOLLIN;
					ev.data.fd= val;
					if (-1== epoll_ctl(epollfd,EPOLL_CTL_ADD,val,&ev))
					{
						Csz::ErrRet("[Peer Manager Run]->failed line:%d,epoll ctl,",__LINE__);
					}
				}
			}
			
			if ((code = epoll_wait(epollfd,events,MAX_EVENTS,10* 1000))>= 0)
			{
				if (0 == code)
				{
					Csz::Close(epollfd);	
					//time out
					Csz::ErrMsg("[Select Switch Run]->time out");
                    if (peer_list.size()< 35)
					{
						break;
                    }
                    else
                    {
                        continue;
                    }
				}
				//3.switch
				int32_t len= 0;
				for (int i= 0; i< code; ++i)
				{
					//TODO time out
					int error_code= recv(events[i].data.fd,(char*)&len,4,MSG_WAITALL);
					//fix bug
					len= ntohl(len);
					if (error_code!= 4)
					{
#ifdef CszTest
                        Csz::LI("[Select Switch Run]->failed,socket recv_code=%d",error_code);
#endif
						peer_manager->CloseSocket(events[i].data.fd);
					}//recv len != 4
					else 
					{
						//thorw exception new
						std::unique_ptr<Parameter> data(new(std::nothrow) Parameter());
						if (nullptr== data)
						{
							//TODO wait,bug socket already take data(eg len and id),should close socket
							Csz::ErrMsg("[Select Switch Run]->failed,new parameter is nullptr");
							peer_manager->CloseSocket(events[i].data.fd);
							continue;
						}
						data->socket= events[i].data.fd;
						//catch message
						//keep alive
						if (0== len)
						{
							DKeepAlive(data.release());
							continue;
						}
						char id;
						error_code= recv(data->socket,&id,1,MSG_DONTWAIT);
						if (error_code!= 1)
						{
							//100ms
							bthread_usleep(100000);
							error_code= recv(data->socket,&id,1,MSG_DONTWAIT);
						}
						if (error_code!= 1)
						{	
							peer_manager->CloseSocket(data->socket);
							continue;
						}
                        
						//normal socket
#ifdef CszTest
						Csz::LI("[Select Switch Run]socket=%d",data->socket);
#endif
						if(0== id)
						{
							DChoke(data.release());
							continue;
						}
						else if(1== id)
						{
							DUnChoke(data.release());
							continue;
						}
						else if(2== id)
						{
							DInterested(data.release());
							continue;
						}
						else if(3== id)
						{
							DUnInterested(data.release());
							continue;
						}
#ifdef CszTest
						Csz::LI("[Select Switch Run]->id=%d,len=%d",(int)id,len-1);
#endif
						//reduce id len
						--len;
						data->buf= new(std::nothrow) char[len];
						if (nullptr== data->buf)
						{
							//TODO wait,bug socket already take data(eg len and id),should close socket
							Csz::ErrMsg("[Select Switch Run]->failed,new buf is nullptr");
							peer_manager->CloseSocket(data->socket);
							continue;
						}
						data->len= len;
						error_code= Csz::RecvTime_us(data->socket,data->buf,data->len,TIMEOUT1000MS);
						if (data->len!= error_code)
						{
							peer_manager->CloseSocket(data->socket);
							continue;
						}
						//success recv all data
						data->cur_len= data->len;
						data->len= 0;
						Task task;
						auto orgin= data.release();
						task.second= orgin;
						if (4== id)
						{
							task.first= &DHave;
							thread_pool->Push(&task);
							//DHave(orgin);
							continue;
						}
						else if (5== id)
						{
							task.first= &DBitField;
							thread_pool->Push(&task);
							//DBitField(orgin);
							continue;
						}
						else if (6== id)
						{
							task.first= &DRequest;
							thread_pool->Push(&task);
							//DRequest(orgin);
							continue;
						}
						else if (7== id)
						{
							if (!(peer_manager->RecvPiece(orgin->socket)))
                            {
                                Csz::ErrMsg("[Select Switch Run]->socket=%d alread recv other index",orgin->socket);
                                continue;
                            }   
							task.first= &DPiece;
							thread_pool->Push(&task);
							//DPiece(orgin);
							continue;
						}
						else if (8== id)
						{
							task.first= &DCancle;
							thread_pool->Push(&task);
							//DCancle(orgin);
							continue;
						}
						else if (9== id)
						{
							task.first= &DPort;
							thread_pool->Push(&task);
							//DPort(orgin);
							continue;
						}
					}//recv len== 4
				}//for
			}//epoll_wait
			else
			{
				Csz::ErrRet("[Select Switch Run]->failed line:%d,",__LINE__);
				break;
			}
			Csz::Close(epollfd);
		}//while
#ifdef CszTest
		//Csz::LI("[Select Switch Run total=%d]",total);
#endif
		return false;
	}

	//TODO thread use 20us,this function use ??and lock other thread lock resource
	inline void SelectSwitch::DKeepAlive(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch keep alive]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		return ;
	}

	void SelectSwitch::DChoke(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch choke]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		PeerManager::GetInstance()->PrChoke(T_data->socket);
		return ;
	}

	void SelectSwitch::DUnChoke(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch unchoke]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		PeerManager::GetInstance()->PrUnChoke(T_data->socket);
		return ;
	}

	void SelectSwitch::DInterested(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch interested]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		PeerManager::GetInstance()->PrInterested(T_data->socket);
		return ;
	}

	void SelectSwitch::DUnInterested(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch uninterested]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		PeerManager::GetInstance()->PrUnInterested(T_data->socket);
		return ;
	}

	void SelectSwitch::DHave(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch have]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		int32_t index= *reinterpret_cast<int32_t*>(T_data->buf);
		//network byte
		index= ntohl(index);
		LocalBitField::GetInstance()->RecvHave(T_data->socket,index);
		return ;
	}

	void SelectSwitch::DBitField(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch bit field]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		LocalBitField::GetInstance()->RecvBitField(T_data->socket,T_data->buf,T_data->cur_len);
		return ;
	}
/*
	void SelectSwitch::AsyncDBitField(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch async bit field]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		//1.go on recv
		int code;
		for (int i= 0; i< 3 && T_data->len> 0; ++i)
		{
			code= recv(T_data->socket,T_data->buf+ T_data->cur_len,T_data->len,MSG_DONTWAIT);
			if (-1== code)
			{
				//not buffer
				if (errno== EAGAIN || errno== EWOULDBLOCK)
				{
					//300ms
					bthread_usleep(300000);
					continue;
				}
				Csz::ErrRet("[Select Switch async bit field]->field");
				break;
			}
			else if (0== code)
			{
				Csz::ErrMsg("[Select Switch async bit field]->failed,can't recv,peer close");
                break;
			}
			T_data->cur_len += code;
			T_data->len -= code;
		}
		//2.1success recv
		if (0== T_data->len)
		{
			//RAII,unique_ptr repeat free memory
            FD_SET(T_data->socket,&rset_save);
			//TODO auto origin= guard.release();
			guard.release();
			DBitField(T_data);
		}
		//2.3 time out,give up socket len < 0
		else if (0== code || T_data->len!= 0)
		{
			PeerManager::GetInstance()->CloseSocket(T_data->socket);
		}
		return ;
	}
*/
	void SelectSwitch::DRequest(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch request]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
        //1.check socket sort 1...4
		if (DownSpeed::GetInstance()->CheckSocket(T_data->socket))
		{
            //network byte
            int32_t index= ntohl(*reinterpret_cast<int32_t*>(T_data->buf));
            //2.have peice
            if (LocalBitField::GetInstance()->CheckBitField(index))
            {
                int32_t begin= ntohl(*reinterpret_cast<int32_t*>(T_data->buf+ 4));
                int32_t length= ntohl(*reinterpret_cast<int32_t*>(T_data->buf+ 8));
                _SendPiece(T_data->socket,index,begin,length);
				return ;
            }                
		}
		return ;
	}
    
    void SelectSwitch::_SendPiece(int T_socket,int32_t T_index,int32_t T_begin,int32_t T_length)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		//1.get have slice file name
		auto file_name= TorrentFile::GetInstance()->GetFileName(T_index,T_begin,T_length);
		if (file_name.empty())
		{
			Csz::ErrMsg("[Select Switch send piece]->failed,file name is empty");
			return ;
		}
		std::string piece_data(T_length,'\0');
		int32_t cur_read= 0;
		for (const auto& val : file_name)
		{
			butil::FilePath file_path(val.first);
			butil::File file(file_path,butil::File::FLAG_OPEN | butil::File::FLAG_READ);
			if (!file.IsValid())
			{
				Csz::ErrMsg("%s",butil::File::ErrorToString(file.error_details()).c_str());
				file.Close();
				break;
			}
			auto read_byte= file.Read(val.second.first,(&piece_data[0])+ cur_read,val.second.second);
			if (val.second.second!= read_byte)
			{
				Csz::ErrMsg("[Select Switch send piece]->failed,length!= read byte,file name %s",val.first.c_str());
				file.Close();
				return ;
			}
            file.Close();
		}
		Piece piece;
		piece.SetParameter(T_index,T_begin,piece_data);
        //2.get mutex
        auto mutex= PeerManager::GetInstance()->GetSocketMutex(T_socket);
        if (nullptr== mutex)
        {
            Csz::ErrMsg("[Select Switch send piece]->failed,return mutex is nullptr");
            return ;
        }
        std::lock_guard<bthread::Mutex> mutex_guard(*mutex);
		send(T_socket,piece.GetSendData(),piece.GetDataSize(),0);
        return ;
    }
    
    //FD_SET socket
	void SelectSwitch::DPiece(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch piece]->parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
        //network byte
        int32_t index= ntohl(*reinterpret_cast<int32_t*>(T_data->buf));
        int32_t begin= ntohl(*reinterpret_cast<int32_t*>(T_data->buf+ 4));
        int32_t length= T_data->cur_len- 8;
        auto local_bit_field= LocalBitField::GetInstance();
        auto torrent_file= TorrentFile::GetInstance();
        //fix bug end_piece,len < slice size
        auto real_end_slice= torrent_file->CheckEndSlice(index,begin);
        if (!real_end_slice.first && SLICESIZE!= length)
        {
            Csz::ErrMsg("[Select Switch piece]->recv piece is error,slice !=%d",SLICESIZE);
            return ;
        }
        if (real_end_slice.first && real_end_slice.second!= length)
        {
            Csz::ErrMsg("[Select Switch piece]->recv piece is error,slice !=%d",real_end_slice.second);
            return ;
        }
        //bug multi thread meanwhile recv one socket
        auto peer_manager= PeerManager::GetInstance();
/*
        if (!(peer_manager->RecvPiece(T_data->socket)))
        {
            Csz::ErrMsg("[Select Switch piece]->socket=%d alread recv other index",T_data->socket);
            return ;
        }   
*/            
        auto need_piece= NeedPiece::GetInstance();
        if (!need_piece->SetIndexW(index))
        {
            peer_manager->ClearPieceStatus(T_data->socket);   
            Csz::ErrMsg("[Select Switch piece]->index alread lock,index=%d",index);
            return ;
        }
        
        //1.write memory
        if (true== BitMemory::GetInstance()->Write(index,begin,T_data->buf+ 8,length))
        {
            DownSpeed::GetInstance()->GetInstance()->AddTotal(T_data->socket,length);
        }
        else
        {
            peer_manager->ClearPieceStatus(T_data->socket);
            need_piece->UnLockIndex(index);
			Csz::ErrMsg("[Select Switch piece]->write memory failed");
            return ;
        }

#ifdef CszTest
        Csz::LI("[Select Switch piece]->success lock index=%d,socket=%d",index,T_data->socket);
#endif

        //2.lock piece
        //std::vector<uint8_t> over(torrent_file->GetPieceBit(index),0);
        //over[begin/SLICESIZE]= 1;
        const int size= torrent_file->GetPieceBit(index);
        auto end_slice= torrent_file->CheckEndSlice(index);

        //if index is end_end,upon write is fill piece space
        //2.1 quick donwload piece
        int code= 0;
		auto mutex= peer_manager->GetSocketMutex(T_data->socket);
		if (nullptr== mutex)
		{
			Csz::ErrMsg("[%s->%d]->failed,socket mutex is nullptr",__func__,__LINE__);
		    need_piece->UnLockIndex(index);
            peer_manager->CloseSocket(T_data->socket);
			return ;
		}
		Request request;
        for (int i= 1; i< size; ++i)
        {
			std::lock_guard<bthread::Mutex> guard(*mutex);
            //2.1.1 end slice
            if ((i== size- 1) && end_slice.first)
            {
				request.SetParameter(index,i* SLICESIZE,end_slice.second);
            }
            //2.1.2 normal slice
            else
            {
				request.SetParameter(index,i* SLICESIZE,SLICESIZE);
            }
			if (send(T_data->socket,request.GetSendData(),request.GetDataSize(),0)!= request.GetDataSize())
			{
				code= -1;
				break;
			}
        }
		//alread save 1 slice
		code= _LockPiece(T_data->socket,index,size- 1);
        if (code> 0 && local_bit_field->CheckBitField(index))
        {
			peer_manager->ClearPieceStatus(T_data->socket);
        }
        else
        {
		    need_piece->UnLockIndex(index);
            peer_manager->CloseSocket(T_data->socket);
        }
        return ;
	}
    
    // bit memory error < 0 | socket close== 0 | success > 0
    inline int8_t SelectSwitch::_LockPiece(int T_socket,int32_t T_index,int32_t T_size)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        int ret= 0;
		auto down_speed= DownSpeed::GetInstance();
        for (int i= 0; i< T_size; )
        {
            int32_t len;
            //int error_code= recv(T_socket,(char*)&len,sizeof(len),0);
		    int error_code= Csz::RecvTime_us(T_socket,(char*)&len,sizeof(len),TIMEOUT6000MS);
			//fix bug
			len= ntohl(len);
			if (sizeof(len)== error_code)
			{
				//thorw exception new
				std::unique_ptr<Parameter> data(new(std::nothrow) Parameter());
				if (nullptr== data)
				{
                    //TODO wait,bug socket already take data(eg len and id),should close socket
					Csz::ErrMsg("[Select Switch lock_piece]->failed,new parameter is nullptr");
					break;
			    }
				data->socket= T_socket;
				//catch message
				//keep alive
				if (0== len)
				{
				    DKeepAlive(data.release());
					continue;
				}
				char id;
				error_code= recv(data->socket,&id,1,MSG_DONTWAIT);
				if (error_code!= 1)
				{
					//1s
					bthread_usleep(1000000);
					error_code= recv(data->socket,&id,1,MSG_DONTWAIT);
				}
				if (error_code!= 1)
				{
					break;
				}
                        
				//normal socket
                if(0== id)
                {
                    DChoke(data.release());
                    continue;
                }
                else if(1== id)
                {
                    DUnChoke(data.release());
                    continue;
                }
                else if(2== id)
                {
                    DInterested(data.release());
                    continue;
                }
                else if(3== id)
                {
                    DUnInterested(data.release());
                    continue;
                }
#ifdef CszTest
                Csz::LI("[Select Switch lock_piece]->id=%d,len=%d,socket=%d",(int)id,len-1,T_socket);
#endif
				//reduce id len
			    --len;
			    data->buf= new(std::nothrow) char[len];
			    if (nullptr== data->buf)
				{
                    //TODO wait,bug socket already take data(eg len and id),should close socket
					Csz::ErrMsg("[Select Switch lock_piece]->failed,new buf is nullptr");
					break;
				}
			    data->len= len;
				error_code= Csz::RecvTime_us(data->socket,data->buf,data->len,TIMEOUT6000MS);
				if (data->len!= error_code)
				{
                    break;
			    }
                //success recv all data
				data->cur_len= data->len;
                data->len= 0;
                //Task task;
                auto orgin= data.release();
                //task.second= orgin;
                if (4== id)
                {
                    //task.first= &DHave;
                    //thread_pool->Push(&task);
					DHave(orgin);
                    continue;
                }
                else if (5== id)
                {
                    //task.first= &DBitField;
                    //thread_pool->Push(&task);
					DBitField(orgin);
                    continue;
                }
                else if (6== id)
                {
                    //task.first= &DRequest;
                    //thread_pool->Push(&task);
					DRequest(orgin);
                    continue;
                }
                else if (7== id)
                {
                    data.reset(orgin);
					PeerManager::GetInstance()->UpdateExpire(data->socket);
                    int32_t index= ntohl(*reinterpret_cast<int32_t*>(data->buf));
                    int32_t begin= ntohl(*reinterpret_cast<int32_t*>(data->buf+ 4));

                    if (T_index== index)
                    {
                        if (BitMemory::GetInstance()->Write(index,begin,data->buf+ 8,data->cur_len- 8))
                        {
							down_speed->AddTotal(data->socket,data->cur_len- 8);
                            ++i;
							ret= 1;
                        }                      
                        else
                        {
                            ret= -1;
							break ;
                        }
                    }
                    else
                    {
                        Csz::ErrMsg("[Select Switch lock_piece]->recv other piece socket=%d,real index=%d,recv index=%d",data->socket,T_index,index);
                        continue;
                    }
                }
                else if (8== id)
                {
                    //task.first= &DCancle;
                    //thread_pool->Push(&task);
                    DCancle(orgin);
                    continue;
                }
                else if (9== id)
                {
                    //task.first= &DPort;
                    //thread_pool->Push(&task);
                    DPort(orgin);
                    continue;
                }
            }
            else
            {
#ifdef CszTest
                Csz::LI("[Select Switch lock_piece]->failed,socket=%d,index=%d,error_code=%d,len=%d,i=%d",T_socket,T_index,error_code,len,i);
#endif
                break;
            }
        }
        return ret;   
    } 

/*
	void SelectSwitch::AsyncDPiece(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch async piece]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		int code = Csz::RecvTime_us(T_data->socket,T_data->buf+ T_data->cur_len,T_data->len,TIMEOUT1000MS);
		if (-1== code)
		{
			Csz::ErrMsg("[Select Switch async piece]->failed,maybe time out or system error");
			return ;
		}
		FD_SET(T_data->socket,&rset_save);
		//TODO auto origin= guard.release();
		guard.release();
		DPiece(T_data);
		return ;
	}
*/
  	
	void SelectSwitch::DCancle(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch cancle]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		return ;
	}

	void SelectSwitch::DPort(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_data)
		{
			Csz::ErrMsg("[Select Switch port]->failed,parameter is nullptr");
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		PeerManager::GetInstance()->UpdateExpire(T_data->socket);
		return ;
	}
}
