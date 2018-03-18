#include <butil/files/file.h> //butil::file
#include <butil/files/file_path.h> //butil::file_path
#include <bthread/bthread.h> //bthread_usleep
#include <butil/time.h> //gettimeofday_s
#include <sys/select.h> //select
#include <sys/socket.h> //getsockopt,setsockopt,recv,send
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

		timeval time_out;
		//5min
		time_out.tv_sec= 10* 60;
		time_out.tv_usec= 0;
		int code;
		//30s
		auto optimistic_start= butil::gettimeofday_s();
		//10s calculate
		auto calculate_start= optimistic_start;
		auto down_speed= DownSpeed::GetInstance();
			
		//2.select
		while (true)
		{
			auto peer_list= std::move(peer_manager->RetSocketList());
			if (peer_list.empty())
			{
				Csz::ErrMsg("[Select Switch Run]->can't switch message type,peer list is empty");
				break;
			}

			//1.set check record
			fd_set rset;
			int fd_max= -1;
			//init
			FD_ZERO(&rset);
			for (const auto& val : peer_list)
			{
				if (val>= 0)
				{
					//set horizontal mark
					int lowat= 4;
					setsockopt(val,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
					if (val > fd_max)
						fd_max= val;
					FD_SET(val,&rset);
				}
			}
			
			if ((code = select(fd_max+ 1,&rset,NULL,NULL,&time_out))>= 0)
			{
				//update status
				auto stop= butil::gettimeofday_s();
				if (stop- calculate_start>= 10)
				{
					down_speed->CalculateSpeed();
					calculate_start= stop;
				}
				if (stop- optimistic_start>= 30)
				{
					peer_manager->Optimistic();
					optimistic_start= stop;
				}
				if (0 == code)
				{
					//time out
					Csz::ErrMsg("[Select Switch Run]->time out");
					break;
				}
				//3.switch
				int32_t len= 0;
				for (const auto& val : peer_list)
				{
#ifdef CszTest
					if (val< 0)
					{
						Csz::LI("[Select Switch Run]->failed,socket< 0");
						continue;
					}
#endif
					if (FD_ISSET(val,&rset))
					{
						--code;
						//TODO time out
						int error_code= recv(val,(char*)&len,4,MSG_WAITALL);
						//fix bug
						len= ntohl(len);
						if (4== error_code)
						{
							//catch message
							//keep alive
							if (0== len)
							{
								DKeepAlive(NULL);
								continue;
							}
							char id;
							error_code= recv(val,&id,1,MSG_DONTWAIT);
							if (error_code!= 1)
							{
								//100ms
								bthread_usleep(100000);
								error_code= recv(val,&id,1,MSG_DONTWAIT);
							}
							if (error_code!= 1)
							{	
								peer_manager->CloseSocket(val);
								continue;
							}
                        
							//normal socket
							//thorw exception new
							std::unique_ptr<Parameter> data(new(std::nothrow) Parameter());
							if (nullptr== data)
							{
								//TODO wait,bug socket already take data(eg len and id),should close socket
								Csz::ErrMsg("[Select Switch Run]->failed,new parameter is nullptr");
								peer_manager->CloseSocket(val);
								continue;
							}
							data->socket= val;
#ifdef CszTest
							Csz::LI("[Select Switch Run]socket=%d",data->socket);
#endif
							if(0== id)
							{
								auto orgin= data.get();
								data.release();
								DChoke(orgin);
								continue;
							}
							else if(1== id)
							{
								auto orgin= data.get();
								data.release();
								DUnChoke(orgin);
								continue;
							}
							else if(2== id)
							{
								auto orgin= data.get();
								data.release();
								DInterested(orgin);
								continue;
							}
							else if(3== id)
							{
								auto orgin= data.get();
								data.release();
								DUnInterested(orgin);
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
							error_code= Csz::RecvTimeP_us(data->socket,data->buf,reinterpret_cast<int32_t*>(&data->len),TIMEOUT1000MS);
							if (-1== error_code)
							{
								peer_manager->CloseSocket(data->socket);
								continue;
							}
							//success recv all data
							data->cur_len= len;
							//Task task;
							auto orgin= data.get();
							data.release();
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
								//task.first= &DPiece;
								//thread_pool->Push(&task);
								DPiece(orgin);
								continue;
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
							peer_manager->CloseSocket(val);
						}//recv len != 4
					}//IS_SET
				}//for
			}//select
		}//while
		if (code < 0)
			Csz::ErrRet("[Select Switch]->failed,not do sure:");
#ifdef CszTest
		Csz::LI("[Select Switch Run total=%d]",total);
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
				Csz::ErrMsg("[Select Switch send piece]->failed,length!= read byte,file name %a",val.first.c_str());
				file.Close();
				return ;
			}
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

        auto need_piece= NeedPiece::GetInstance();
        if (!need_piece->SetIndexW(index))
        {
            Csz::ErrMsg("[Select Switch piece]->index alread lock,index=%d",index);
            return ;
        }

        auto down_speed= DownSpeed::GetInstance()->GetInstance();
        //1.write memory
        if (true== BitMemory::GetInstance()->Write(index,begin,T_data->buf+ 8,length))
        {
            down_speed->AddTotal(T_data->socket,length);
        }
        else
        {
            need_piece->UnLockIndex(index);
			Csz::ErrMsg("[Select Switch piece]->write memory failed");
            return ;
        }

        //2.lock piece
        std::vector<uint8_t> over(torrent_file->GetPieceBit(index),0);
        over[begin/SLICESIZE]= 1;
        int fill= 1;
        const int size= over.size();
        auto end_slice= torrent_file->CheckEndSlice(index);

        //if index is end_end,upon write is fill piece space
        //2.1 quick donwload piece
        int code= 0;
        while (!local_bit_field->CheckBitField(index) && fill< size)
        { 
            int32_t cur_write= 0;
            for (int i= 0; i< size; ++i)
            {
                //lack slice
                if (over[i]== 0)
                {   
                    //2.1.1 end slice
                    if ((i== size- 1) && end_slice.first)
                    {
                        code= _LockPiece(T_data->socket,index,i* SLICESIZE,end_slice.second);
                        cur_write= end_slice.second;
                    }
                    //2.1.2 normal slice
                    else
                    {
                        code= _LockPiece(T_data->socket,index,i* SLICESIZE,SLICESIZE);
                        cur_write= SLICESIZE;
                    }
                    //success recv slice
                    if (code> 0)
                    {
                        down_speed->AddTotal(T_data->socket,cur_write);
                        over[i]= 1;
                        ++fill;
                    }
                    else if (code<= 0)
                    {   
                        break;   
                    }
                }
            }
            if (code< 0)
            {    
                break;
            }  
        }
        if (code> 0 && fill== size && local_bit_field->CheckBitField(index))
        {
			;
        }
        else
        {
		    need_piece->UnLockIndex(index);
            PeerManager::GetInstance()->CloseSocket(T_data->socket);
        }
        return ;
	}
    
    // bit memory error < 0 | socket close== 0 | success > 0
    inline int8_t SelectSwitch::_LockPiece(int T_socket,int32_t T_index,int32_t T_begin,int32_t T_length)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        {
            Request request;
            request.SetParameter(T_index,T_begin,T_length);
            auto mutex= PeerManager::GetInstance()->GetSocketMutex(T_socket);
            if (nullptr== mutex)
            {
                Csz::ErrMsg("[Select Switch look piece]->failed,return mutex is nullptr");
                return 0;
            }
            std::lock_guard<bthread::Mutex> mutex_guard(*mutex);
            if (send(T_socket,request.GetSendData(),request.GetDataSize(),0)!= T_length)
            {
                Csz::ErrMsg("[Select Switch lock piece]->failed,send size!=%d",T_length);
                return 0;
            }
        }
        int ret= 0;
        for (int i= 0; i< 3; ++i)
        {
            int32_t len;
		    int error_code= Csz::RecvTime_us(T_socket,(char*)&len,sizeof(len),TIMEOUT3000MS);
			//fix bug
			len= ntohl(len);
			if (4== error_code)
			{
				//catch message
				//keep alive
				if (0== len)
				{
				    DKeepAlive(NULL);
					continue;
				}
				char id;
				error_code= recv(T_socket,&id,1,MSG_DONTWAIT);
				if (error_code!= 1)
				{
					//1s
					bthread_usleep(1000000);
					error_code= recv(T_socket,&id,1,MSG_DONTWAIT);
				}
				if (error_code!= 1)
				{
					break;
				}
                        
				//normal socket
				//thorw exception new
				std::unique_ptr<Parameter> data(new(std::nothrow) Parameter());
				if (nullptr== data)
				{
                    //TODO wait,bug socket already take data(eg len and id),should close socket
					Csz::ErrMsg("[Select Switch lock piece]->failed,new parameter is nullptr");
					break;
			    }
				data->socket= T_socket;
                        
                if(0== id)
                {
                    auto orgin= data.get();
                    data.release();
                    DChoke(orgin);
                    continue;
                }
                else if(1== id)
                {
                    auto orgin= data.get();
                    data.release();
                    DUnChoke(orgin);
                    continue;
                }
                else if(2== id)
                {
                    auto orgin= data.get();
                    data.release();
                    DInterested(orgin);
                    continue;
                }
                else if(3== id)
                {
                    auto orgin= data.get();
                    data.release();
                    DUnInterested(orgin);
                    continue;
                }
#ifdef CszTest
                Csz::LI("[Select Switch lock piece]->id=%d,len=%d",(int)id,len-1);
#endif
				//reduce id len
			    --len;
			    data->buf= new(std::nothrow) char[len];
			    if (nullptr== data->buf)
				{
                    //TODO wait,bug socket already take data(eg len and id),should close socket
					Csz::ErrMsg("[Select Switch lock piece]->failed,new buf is nullptr");
					break;
				}
			    data->len= len;
				error_code= Csz::RecvTime_us(data->socket,data->buf,data->len,TIMEOUT3000MS);
				if (-1== error_code)
				{
                    break;
			    }
                //success recv all data
                data->cur_len= len;
                //Task task;
                auto orgin= data.get();
                data.release();
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
                    int32_t index= ntohl(*reinterpret_cast<int32_t*>(data->buf));
                    int32_t begin= ntohl(*reinterpret_cast<int32_t*>(data->buf+ 4));
                    if (T_index== index && T_begin== begin && T_length== data->cur_len- 8)
                    {
                        if (BitMemory::GetInstance()->Write(index,begin,data->buf+ 8,T_length))
                        {
                            ret= 1;
                        }                      
                        else
                        {
                            ret= -1;
                        }
                        break;
                    }
                    else
                    {
                        Csz::ErrMsg("[Select Switch lock piece]->recv other piece");
                        //TODO delete i
                        --i;
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
		return ;
	}

/*
	void* SelectSwitch::RequestRuner(void*)
	{
		auto local_bit_field= LocalBitField::GetInstance();
		auto need_piece= NeedPiece::GetInstance();
		while (!local_bit_field->GameOver() && !need_piece->RunnerStatus())
		{
			need_piece->Runner();
		}
		return nullptr;
	}
*/

}
