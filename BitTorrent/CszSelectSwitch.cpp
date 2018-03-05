#include "CszBitTorrent.h"
#include <butil/files/file.h> 
#include <butil/files/file_path.h>

namespace Csz
{
	//TODO sinnal deal or pselect
	bool SelectSwitch::operator()() const
	{
		using Task= Csz::TaskQueue<SelectSwitch::Parameter,THREADNUM>::Type;
		auto peer_manager= PeerManaget::GetInstance();
		auto& peer_list= peer_manager->RetSocketList();
		//auto need_piece= NeedPiece::GetInstance();
		auto thread_pool= SingleTonTHread<SelectSwitch::Parameter,THREADNUM>::GetInstance();
		auto thread_pool
		if (peer_list.empty())
		{
			Csz::ErrMsg("select can't switch message type,peer list is empty");
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
		rest= rest_save;
		timeval time_out;
		time_out.tv_sec= 60;
		time_out.tv_uset= 0;
		int code;
		//2.select
		while ((code= select(fd_max+ 1,&rest,NULL,NULL,&time_out))>= 0)
		{
			if (0 == code)
			{
				//time out
				Csz::ErrMsg("Select Switch time out");
				break;
			}
			//3.switch
			int32_t len= 0;
			for (const auto& val : peer_list)
			{
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
							DKeepAlice(nullptr);
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
						//reduce id len
						--len;
						//thorw exception new
						Parameter* data= new(std::nothrow) Parameter();
						if (nullptr== data)
						{
							Csz::ErrMsg("Select Switch operator() failed,new parameter is nullptr");
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
						data->buf= new(std::nothrow) char[len];
						if (nullptr== data->buf)
						{
							Csz::ErrMsg("Select Switch operator() failed,new buf is nullptr");
							continue;
						}
						data->len= len;
						error_code= Csz::RecvTimeP_us(data->socket,data->buf,&data->len,TIMEOUT300MS);
						if (error_code== -1)
						{
							if (errno== EAGAIN || errno== EWOULDBLOCK)
							{
								if (5== id)
								{
									Task task(std::make_pair(&AsynDBitField,data);
									thread_pool.Push(task);
									continue;
								}
								else if (7== id)
								{
									Task task(std::make_pair(&AsyncDPiece,data));
									thread_Pool.Push(task);
								}
							}
						}
					}
				}
			}
		}
		if (code < 0)
			Csz::ErrRet("Select Switch not do sure:");
		return false;
	}

	//TODO thread use 20us,this function use ??and lock other thread lock resource
	inline void SelectSwitch::DKeepAlive(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch keep alive,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
	}

	inline void SelectSwitch::DChoke(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch choke,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPChoke(T_data->socket);
		return ;
	}

	inline void SelectSwitch::DUnChoke(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch unchoke,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPUnChoke(T_data->socket);
		return ;
	}

	inline void SelectSwitch::DInterested(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch interested,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPInterested(T_data->socket);
		return ;
	}

	inline void SelectSwitch::DUnInterested(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch uninterested,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPUnInterested(T_data->socket);
		return ;
	}

	static void SelectSwitch::DHave(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch have,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		int32_t index= *reinterpret_cast<int32_t*>(T_data->buf);
		//network byte
		index= ntohl(index);
		LocalBitField::GetInstance()->RecvHave(T_data->socket,index);
		return ;
	}

	static void SelectSwitch::DBitField(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch bit field,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		LoaclBitField::GetINstance()->RecvBitField(T_data->socket,T_data->buf,T_data->cur_len);
		return ;
	}

	static void SelectSwitch::AsyncDBitField(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch async bit field,parameter is nullptr")
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
				Csz::ErrRet("Select Switch async recv bit field");
				break;
			}
			else if (0== code)
			{
				Csz::ErrMsg("Select Switch can't recv,peer close");
                break;
			}
			T_data->cur_len += code;
			T_data->len -= code;
		}
		//2.1success recv
		if (0== T_data->len)
		{
			//RAII,unique_ptr repeat free memory
            FD_SET(T_data->socket,rset_save);
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

	static void SelectSwitch::DRequest(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch request,parameter is nullptr")
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
                int32_t length= ntohl(*reinterpret_cast<int32_t*>(T_data->buf+ 8);
                _SendPiece(T_data->socket,index,begin,length);
				return ;
            }                
		}
		return ;
	}
    
    void SelectSwitch::_SendPiece(int T_socket,int32_t T_index,int32_t T_begin,int32_t T_length)
    {
		//1.get have slice file name
		auto file_name= TorrentFile::GetInstance()->GetFileName(T_index,T_begin+ T_length);
		if (file_name.empty())
		{
			Csz::ErrMsg("Select Switch send piece file name is empty");
			return ;
		}
		std::string piece_data(T_length,'\0');
		int32_t cur_read= 0;
		for (const auto& val : file_name)
		{
			butil::FilePath file_path(val.first);
			butil::File file(file_path,butil::File::FLAG_OPEN | butil::FLAG_READ);
			if (!file.IsValid())
			{
				Csz::ErrMsg("%s",butil::ErrorToString(file.error_details()).c_str());
				file.Close();
				break;
			}
			auto read_byte= file.Read(val.second.first,(&piece_data[0])+ cur_read,val.second.second);
			if (val.second.second!= read_byte)
			{
				Csz::ErrMsg("Select Switch send piece length!= read byte,file name %a",val.first.c_str());
				file.Close();
				return ;
			}
		}
		Piece piece;
		piece.SetParameter(T_index,T_begin,piece_data);
        //2.get mutex
        auto mutex= PeerManager::GetInstance()->GetSocketMutex(T_data->socket);
        if (nullptr== mutex)
        {
            Csz::ErrMsg("Select Switch send piece failed,return mutex is nullptr");
            return ;
        }
        std::unique_lock<bthread::Mutex> mutex_guard(*mutex);
		send(T_socket,piece.GetSendData(),piece.GetDataSize(),0);
        return ;
    }
    
    //FD_SET socket
	void SelectSwitch::DPiece(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch piece,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
        //network byte
        int32_t index= ntohl(*reinterpret_cast<int32_t*>(T_data->buf));
        int32_t begin= ntohl(*reinterpret_cast<int32_t*>(T_data->buf+ 4));
        int32_t length= T_data->cur_len- 8;
        auto local_bit_field= LocalBitField::GetInstance();

        //fix bug end_piece,len < slice size
        auto real_end_slice= local_bit_field->CheckEndSlice(index,begin);
        if (!real_end_slice.first && SLICESIZE!= length)
        {
            Csz::ErrMsg("Select Switch recv piece is error,slice !=%d",SLICESIZE);
            return ;
        }
        if (real_end_slice.first && real_end_slice.second!= length)
        {
            Csz::ErrMsg("Select Switch recv piece is error,slice !=%d",real_end_slice.second);
            return ;
        }

        //1.write memory
        auto memory_pool= BitMemory::GetInstance();
        memory_pool->Write(index,begin,T_data->buf+ 8,length);

        //2.lock piece
        auto need_piece= NeedPiece::GetInstance();
        auto peer_manager= PeerManager::GetInstance();
        std::vector<uint8_t> over(TorrentFile::GetInstance()->GetPieceBit(index),0);
        over[begin/SLICESIZE]= 1;
        int cur_socket= T_data->socket;
        int fill= 1;
        const int size= over.size();
        auto end_slice= local_bit_field->CheckEndSlice(index);

        //if index is end_end,upon write is fill piece space
        //2.1 quick donwload piece
        std::vector<int> sockets_alive;
        while (!local_bit_field->CheckBitField(index) && fill< size)
        { 
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
                    }
                    //2.1.2 normal slice
                    else
                    {
                        code= _LockPiece(cur_socket,index,i* SLICESIZE,T_SLICESIZE);
                    }
                    //success recv slice
                    if (true== code)
                    {
                        over[i]= 1;
                        ++fill;
                    }
                    // failed socket,unoder
                    else if (!sockets_alive.empyt())
                    {
                        //close failed socket
                        peer_manager->CloseSocket(cur_socket);

                        cur_socket= sockets_alive.back();
                        socket_alive.bakc_pop();
                    }
                    else
                    {
                        //close failed socket
                        peer_manager->CloseSocket(cur_socket);

                        //init cur_socket
                        cur_socket= -1;

                        //notify peer mamager
                        peer_mamager->CloseSocket(cur_socket);

                        //get new hot socket for piece
                        int time_out= TIMEOUT1000MS;
                        for (int num= 0; num< 7; ++num)
                        {
                            sockets_alive= std::move(need_piece->PopPointNeed(index));
                            if (sockets_alive.empty())
                            {
                                Csz::ErrMsg("Select Switch deal piece time out,pop point need piece");
                                bthread_usleep(time_out);
                                time_cout*= 2;   
                            }
                            else
                            {
                                cur_socket= sockets_alive.back();
                                sockets_alive.back_pop();
                                break;
                            }
                        }
                        if (cur_socket< 0)
                        {
                            break;
                        }
                    }
                }
            }
            if (cur_socket< 0)
            {    
                BitMemory::GetInstance()->ClearIndex(index);
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
                Csz::ErrMsg("Select Switch look piece failed,return mutex is nullptr");
                return false;
            }
            std::unique_lock<bthread::Mutex> mutex_guard(*mutex);
            if (send(T_socket,request.GetSendData(),request.GetDataSize(),0)!= T_length)
            {
                Csz::ErrMsg("Select Switch lock piece failed,send size!=%d",T_length)
                return false;
            }
        }
        bool ret= false;
        for (int i= 0; i< 3; ++i)
        {
            Parameter* parameter;
            std::unique_ptr<Parameter> guard(new Parameter);
            parameter->socket= T_socket;
            int code= Csz::RecvTime_us(parameter->socket,&parameter->len,sizeof(parameter->len),TIMEOUT1000MS);
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
                Csz::ErrMsg("Select Switch lock piece recv len < 0");
                break;
            }
            char id;
            --parameter->len;
            parameter->buf= new(std::nothrow) char[parameter->len];
            if (nullptr== parameter->buf)
            {
                Csz::ErrMsg("Select Switch lock piece new failed");
                break;
            }
            code= Csz::RecvTime_us(parameter->socket,&id,1,TIMEOUT1000MS);
            if (-1== code)
            {
                break;
            }
            if (0== id)//catch choke
            {
                break;
            }
            else if (1== id)//catch unchoke
            {
                NeedPiece::GetInstance()->NPUnChoke(parameter->socket);
            }
            else if (2== id)//catch interested
            {
                NeedPiece::GetInstance()->NPInterested(parameter->socket);
            }
            else if (3== id)//catch not interested
            {
                NeedPiece::GetInstance()->NPUnInterester(parameter->socket);
            }
            else if (4== id)//catch have
            {
                if (parameter->len!= 4)
                {
                    Csz::ErrMsg("Select Switch lock piece recv have,but len!= 4");
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
                Csz::ErrMsg("Select Switch lock piece recv bit field");
            }
            else if (6== id)//catch request
            {   
                if (parameter->len!= 12)
                {
                    Csz::ErrMsg("Select Switch lock piece recv request,but len!= 12");
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
                    Csz::ErrMsg("Select Switch lock piece recv piece,but index is mismatch");
                    continue;
                }
                int32_t begin= ntohl(*reinterpret_cast<int32_t*>(parameter->buf+ 4));
                if (T_begin!= begin)
                {
                    continue;   
                }
                BitMemory::GetInstance()->Write(index,begin,parameter->buf+ 8,parameter->len -8);
                ret= true;
                break;
            }
            else if (8== id)//catch cancle
            {
                if (parameter->len!= 12)
                {
                    Csz::ErrMsg("Select Switch lock piece recv cancle,but len!= 12");
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
                    Csz::ErrMsg("Select Switch lock piece recv port,but len!= 2");
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
                Csz::ErrMsg("Select Switch lock piece recv unknow id%d",(int)id);
            }
        }
        return ret;   
    } 

	void SelectSwitch::AsyncDPiece(Parameter* T_data)
	{
		if (nullptr== T_data)
		{
			Csz::ErrMsg("Select Switch,parameter is nullptr")
			return ;
		}
		std::unique_ptr<Parameter> guard(T_data);
		int code = Csz::RecvTime_us(T_data->socket,T_data->buf+ T_data->cur_len,T_data->len,TIMEOUT1000MS);
		if (-1== code)
		{
			Csz::ErrMSg("Select Switch async piece failed");
			return ;
		}
		FD_SET(T_data->socket,rset_save);
		auto origin= guard.release();
		DPiece(origin);
		return ;
	}	
}
