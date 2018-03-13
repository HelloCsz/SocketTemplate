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
    fd_set SelectSwitch::rset_save;
	//TODO sinnal deal or pselect
	bool SelectSwitch::operator()()
	{
		using Task= Csz::TaskQueue<SelectSwitch::Parameter,THREADNUM>::Type;
		auto thread_pool= SingletonThread<SelectSwitch::Parameter,THREADNUM>::GetInstance();
		auto peer_manager= PeerManager::GetInstance();
		auto peer_list= std::move(peer_manager->RetSocketList());
		//auto need_piece= NeedPiece::GetInstance();
		if (peer_list.empty())
		{
			Csz::ErrMsg("[Select Switch Run]->can't switch message type,peer list is empty");
			return false;
		}

		//1.set check record
		fd_set rset;
		FD_ZERO(&rset_save);
		int fd_max= -1;
		//init
		for (const auto& val : peer_list)
		{
			if (val>= 0)
			{
				//set horizontal mark
				int lowat= 4;
				setsockopt(val,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
				if (val > fd_max)
					fd_max= val;
				FD_SET(val,&rset_save);
			}
		}
		rset= rset_save;
		timeval time_out;
		//3min
		time_out.tv_sec= 180;
		time_out.tv_usec= 0;
		int code;
		//30s
		auto optimistic_start= butil::gettimeofday_s();
		//10s calculate
		auto calculate_start= optimistic_start;
		auto down_speed= DownSpeed::GetInstance();
		//2.select
		while ((code= select(fd_max+ 1,&rset,NULL,NULL,&time_out))>= 0)
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
                if (code<= 0)
                    break;
                if (val< 0)
                    continue;
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
							FD_CLR(val,&rset_save);
							peer_manager->CloseSocket(val);
							continue;
						}
						//normal socket
						//thorw exception new
						Parameter* data= new(std::nothrow) Parameter();
						if (nullptr== data)
						{
                            //TODO wait,bug socket already take data(eg len and id),should close socket
							Csz::ErrMsg("[Select Switch Run]->failed,new parameter is nullptr");
                            peer_manager->CloseSocket(val);
                            FD_CLR(val,&rset_save);
							continue;
						}
						data->socket= val;
						if (0== id)
						{
							DChoke(data);
							continue;
						}
						else if (1== id)
						{
							DUnChoke(data);
							continue;
						}
						else if (2== id)
						{
							DInterested(data);
							continue;
						}
						else if (3== id)
						{
							DUnInterested(data);
							continue;
						}
						//reduce id len
						--len;
						data->buf= new(std::nothrow) char[len];
						if (nullptr== data->buf)
						{
                            //TODO wait,bug socket already take data(eg len and id),should close socket
							Csz::ErrMsg("[Select Switch Run]->failed,new buf is nullptr");
                            peer_manager->CloseSocket(data->socket);
                            FD_CLR(data->socket,&rset_save);
							continue;
						}
						data->len= len;
						error_code= Csz::RecvTimeP_us(data->socket,data->buf,reinterpret_cast<size_t*>(&data->len),TIMEOUT300MS);
						if (error_code== -1)
						{
							if (errno== EAGAIN || errno== EWOULDBLOCK)
							{
                                data->cur_len= len- data->len;
								if (5== id)
								{
									//Task task(std::make_pair(&AsyncDBitField,data));
									//thread_pool->Push(&task);
									AsyncDBitField(data);
                                    continue;
								}
								else if (7== id)
								{
									//Task task(std::make_pair(&AsyncDPiece,data));
									//thread_pool->Push(&task);
									AsyncDPiece(data);
                                    continue;
								}
							}
                            peer_manager->CloseSocket(data->socket);
                            FD_CLR(data->socket,&rset_save);
                            continue;
						}
                        //success recv all data
                        data->cur_len= len;
                        Task task;
                        task.second= data;
                        if (4== id)
                        {
                            //task.first= &DHave;
                            //thread_pool->Push(&task);
							DHave(data);
                            continue;
                        }
                        else if (5== id)
                        {
                            //task.first= &DBitField;
                            //thread_pool->Push(&task);
							DBitField(data);
                            continue;
                        }
                        else if (6== id)
                        {
                            //task.first= &DRequest;
                            //thread_pool->Push(&task);
							DRequest(data);
                            continue;
                        }
                        else if (7== id)
                        {
                            //task.first= &DPiece;
                            //thread_pool->Push(&task);
							DPiece(data);
                            continue;
                        }
                        else if (8== id)
                        {
                            //task.first= &DCancle;
                            //thread_pool->Push(&task);
                            continue;
                        }
                        else if (9== id)
                        {
                            //task.first= &DPort;
                            //thread_pool->Push(&task);
                            continue;
                        }
					}
				}
			}
		}
		if (code < 0)
			Csz::ErrRet("[Select Switch]->failed,not do sure:");
		return false;
	}

	//TODO thread use 20us,this function use ??and lock other thread lock resource
	inline void SelectSwitch::DKeepAlive(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[Select Switch]->keep alive");
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
        Csz::LI("[Select Switch]->choke");
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
        Csz::LI("[Select Switch]->unchoke");
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
        Csz::LI("[Select Switch]->interested");
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
        Csz::LI("[Select Switch]->uninterested");
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
        Csz::LI("[Select Switch]->have");
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
        Csz::LI("[Select Switch]->bit field");
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

	void SelectSwitch::AsyncDBitField(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[Select Switch]->async bit field");
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
			auto origin= guard.release();
			DBitField(origin);
		}
		//2.3 time out,give up socket len < 0
		else if (0== code || T_data->len!= 0)
		{
			PeerManager::GetInstance()->CloseSocket(T_data->socket);
		}
		return ;
	}

	void SelectSwitch::DRequest(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[Select Switch]->request");
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
        std::unique_lock<bthread::Mutex> mutex_guard(*mutex);
		send(T_socket,piece.GetSendData(),piece.GetDataSize(),0);
        return ;
    }
    
    //FD_SET socket
	void SelectSwitch::DPiece(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[Select Switch]->piece");
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

        auto down_speed= DownSpeed::GetInstance()->GetInstance();
        //1.write memory
        if (true== BitMemory::GetInstance()->Write(index,begin,T_data->buf+ 8,length))
        {
            down_speed->AddTotal(T_data->socket,length);
        }
        else
        {	
			Csz::ErrMsg("[Select Switch piece]->write memory failed");
            return ;
        }

        //2.lock piece
        auto need_piece= NeedPiece::GetInstance();
        auto peer_manager= PeerManager::GetInstance();
        std::vector<uint8_t> over(torrent_file->GetPieceBit(index),0);
        over[begin/SLICESIZE]= 1;
        int cur_socket= T_data->socket;
        int fill= 1;
        const int size= over.size();
        auto end_slice= torrent_file->CheckEndSlice(index);

        //if index is end_end,upon write is fill piece space
        //2.1 quick donwload piece
        std::vector<int> sockets_alive;
        while (!local_bit_field->CheckBitField(index) && fill< size)
        { 
            int32_t cur_write= 0;
            for (int i= 0; i< size; ++i)
            {
                //lack slice
                if (over[i]== 0)
                {   
                    int code;
                    //2.1.1 end slice
                    if ((i== size- 1) && end_slice.first)
                    {
                        code= _LockPiece(cur_socket,index,i* SLICESIZE,end_slice.second);
                        cur_write= end_slice.second;
                    }
                    //2.1.2 normal slice
                    else
                    {
                        code= _LockPiece(cur_socket,index,i* SLICESIZE,SLICESIZE);
                        cur_write= SLICESIZE;
                    }
                    //success recv slice
                    if (true== code)
                    {
                        down_speed->AddTotal(cur_socket,cur_write);
                        over[i]= 1;
                        ++fill;
                    }
                    // failed socket,unoder
                    else if (!sockets_alive.empty())
                    {
                        //close failed socket
                        peer_manager->CloseSocket(cur_socket);

                        cur_socket= sockets_alive.back();
                        sockets_alive.pop_back();
                    }
                    else
                    {
                        //close failed socket
                        peer_manager->CloseSocket(cur_socket);

                        //init cur_socket
                        cur_socket= -1;

                        //get new hot socket for piece
                        int time_out= TIMEOUT1000MS;
                        for (int num= 0; num< 7; ++num)
                        {
                            sockets_alive= std::move(need_piece->PopPointNeed(index));
                            if (sockets_alive.empty())
                            {
                                Csz::ErrMsg("[Select Switch piece]->failed,pop point need return alive socket is empty");
                                bthread_usleep(time_out);
                                time_out*= 2;   
                            }
                            else
                            {
                                cur_socket= sockets_alive.back();
                                sockets_alive.pop_back();
                                break;
                            }
                        }
                        if (cur_socket< 0)
                        {
							need_piece->UnLockIndex(index);
							Csz::ErrMsg("[Select Switch dpiece]->failed,wait connected socket time out");
                            break;
                        }
					}
                }
            }
            if (cur_socket< 0)
            {    
                break;
            }  
        }
        return ;
	}
    
    inline bool SelectSwitch::_LockPiece(int T_socket,int32_t T_index,int32_t T_begin,int32_t T_length)
    {
        {
            Request request;
            request.SetParameter(T_index,T_begin,T_length);
            auto mutex= PeerManager::GetInstance()->GetSocketMutex(T_socket);
            if (nullptr== mutex)
            {
                Csz::ErrMsg("[Select Switch look piece]->failed,return mutex is nullptr");
                return false;
            }
            std::unique_lock<bthread::Mutex> mutex_guard(*mutex);
            if (send(T_socket,request.GetSendData(),request.GetDataSize(),0)!= T_length)
            {
                Csz::ErrMsg("[Select Switch lock piece]->failed,send size!=%d",T_length);
                return false;
            }
        }
        bool ret= false;
        for (int i= 0; i< 3; ++i)
        {
            Parameter* parameter= new(std::nothrow) Parameter();
            if (nullptr== parameter)
            {
                Csz::ErrMsg("[Select Switch lock piece]->failed,new Parameter return nullptr");
                continue;
            }
            std::unique_ptr<Parameter> guard(parameter);
            parameter->socket= T_socket;
            int code= Csz::RecvTime_us(parameter->socket,(char*)&parameter->len,sizeof(parameter->len),TIMEOUT1000MS);
            if (-1== code)
            {
                break;
            }
            //catch keep alive
            if (0== parameter->len)
            {
                continue;
            }
            //network byte
            parameter->len= ntohl(parameter->len);
            if (parameter->len< 0)
            {
                Csz::ErrMsg("[Select Switch lock piece]->failed,recv len < 0");
                break;
            }
            char id;
            --parameter->len;
            parameter->buf= new(std::nothrow) char[parameter->len];
            if (nullptr== parameter->buf)
            {
                Csz::ErrMsg("[Select Switch lock piece]->new parameter failed");
                break;
            }
            code= Csz::RecvTime_us(parameter->socket,&id,1,TIMEOUT1000MS);
            if (-1== code)
            {
                break;
            }
            if (0== id)//catch choke
            {
				guard.release();
				DChoke(parameter);
                break;
            }
            else if (1== id)//catch unchoke
            {
				guard.release();
				DUnChoke(parameter);
                //NeedPiece::GetInstance()->UnChoke(parameter->socket);
            }
            else if (2== id)//catch interested
            {
				guard.release();
				DInterested(parameter);
                //NeedPiece::GetInstance()->NPInterested(parameter->socket);
            }
            else if (3== id)//catch not interested
            {
				guard.release();
				DUnInterested(parameter);
                //NeedPiece::GetInstance()->NPUnInterested(parameter->socket);
            }
            else if (4== id)//catch have
            {
                if (parameter->len!= 4)
                {
                    Csz::ErrMsg("[Select Switch lock piece]->failed,recv have,but len!= 4");
                    break;
                }
                code= Csz::RecvTime_us(parameter->socket,parameter->buf,parameter->len,TIMEOUT1000MS);
                if (-1== code)
                    break;
                parameter->len-= code;
                parameter->cur_len+= code;
                guard.release();
                DHave(parameter);
            }
            else if (5== id)//catch bit field
            {
                Csz::ErrMsg("[Select Switch lock piece]->falied,recv bit field");
            }
            else if (6== id)//catch request
            {   
                if (parameter->len!= 12)
                {
                    Csz::ErrMsg("[Select Switch lock piece]->falied,recv request,but len!= 12");
                    break;
                }
                code= Csz::RecvTime_us(parameter->socket,parameter->buf,parameter->len,TIMEOUT1000MS);
                if (-1== code)
                    break;
                parameter->len-= code;
                parameter->cur_len+= code;
                guard.release();
                DRequest(parameter);
            }
            else if (7== id)//catch piece
            {
                if (T_length!= (parameter->len- 8))
                {
                    break;
                }
                code= Csz::RecvTime_us(parameter->socket,parameter->buf,parameter->len,TIMEOUT3000MS);
                if (-1== code)
                    break;
                int32_t index= ntohl(*reinterpret_cast<int32_t*>(parameter->buf));
                if (T_index!= index)
                {
                    //TODO recv
                    Csz::ErrMsg("[Select Switch lock piece]->falied,recv piece,but index is mismatch");
                    continue;
                }
                int32_t begin= ntohl(*reinterpret_cast<int32_t*>(parameter->buf+ 4));
                if (T_begin!= begin)
                {
                    //TODO recv
                    continue;   
                }
                if (true== BitMemory::GetInstance()->Write(index,begin,parameter->buf+ 8,parameter->len -8))
                    ret= true;
                break;
            }
            else if (8== id)//catch cancle
            {
                if (parameter->len!= 12)
                {
                    Csz::ErrMsg("[Select Switch lock piece]->failed,recv cancle,but len!= 12");
                    break;
                }
                code= Csz::RecvTime_us(parameter->socket,parameter->buf,parameter->len,TIMEOUT1000MS);
                if (-1== code)
                    break;
                parameter->len-= code;
                parameter->cur_len+= code;
                guard.release();
                DCancle(parameter);
            }
            else if (9== id)//catch port
            {
                if (parameter->len!= 2)
                {
                    Csz::ErrMsg("[Select Switch lock piece]->failed,recv port,but len!= 2");
                    break;
                }
                code= Csz::RecvTime_us(parameter->socket,parameter->buf,parameter->len,TIMEOUT1000MS);
                if (-1== code)
                    break;
                parameter->len-= code;
                parameter->cur_len+= code;
                guard.release();
                DPort(parameter);
            }
            else
            {
                Csz::ErrMsg("[Select Switch lock piece]->failed,recv unknow id%d",(int)id);
            }
        }
        return ret;   
    } 

	void SelectSwitch::AsyncDPiece(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[Select Switch]->async piece");
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
		auto origin= guard.release();
		DPiece(origin);
		return ;
	}
    	
	void SelectSwitch::DCancle(Parameter* T_data)
	{
#ifdef CszTest
        Csz::LI("[Select Switch]->cancle");
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
        Csz::LI("[Select Switch]->port");
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
