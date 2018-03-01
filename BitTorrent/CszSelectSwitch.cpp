#include "CszBitTorrent.h"
#include <butil/files/file.h> 
#include <butil/files/file_path.h>

namespace Csz
{
	//TODO sinnal deal or pselect
	bool SelectSwitch::operator()() const
	{
		auto& peer_list= PeerManager::GetInstance()->RetSocketList();
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
						//thorw exception new
						std::unique_ptr<char[]> message(new char[len]);
					}
				}
			}
		}
		if (code < 0)
			Csz::ErrRet("Select Switch not do sure:");
		return false;
	}

	static void SelectSwitch::DKeepAlive(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
		std::unique_ptr<Parameter> guard(T_data);
	}

	static void SelectSwitch::DChoke(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPChoke(T_data->socket);
		return ;
	}

	static void SelectSwitch::DUnChoke(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPUnChoke(T_data->socket);
		return ;
	}

	static void SelectSwitch::DInterested(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPInterested(T_data->socket);
		return ;
	}

	static void SelectSwitch::DUnInterested(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
		std::unique_ptr<Parameter> guard(T_data);
		NeedPiece::GetInstance()->NPUnInterested(T_data->socket);
		return ;
	}

	static void SelectSwitch::DHave(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
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
			return ;
		std::unique_ptr<Parameter> guard(T_data);
		LoaclBitField::GetINstance()->RecvBitField(T_data->socket,T_data->buf,T_data->cur_len);
		return ;
	}

	static void SelectSwitch::AsyncDBitField(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
		std::unique_ptr<Parameter,std::function<void(Parameter*)>> guard(T_data);
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
            FD_SET(origin->socket,rset_save);
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
			return ;
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
		auto torrent_file= TorrentFile::GetInstance();
		auto file_name= torrent_file->GetFileName(T_index,T_begin+ T_length);
		if (file_name.empty())
		{
			Csz::ErrMsg("Select Switch send piece file name is empty");
			return ;
		}
		//TODO need do multi file
		//1.single
		std::string piece_data(T_length,'\0');
		butil::FilePath file_path(file_name[0]);
		butil::File file(file_path,butil::File::FLAG_OPEN | butil::FLAG_READ);
		auto offset= torrent_file->GetOffSetOf(T_index);
		auto read_byte= file.Read(offset,&piece_data[0],T_length);
		if (T_length!= read_byte)
		{
			Csz::ErrMsg("Select Switch send piece length!= read byte");
			return ;
		}
		Piece piece;
		piece.SetParameter(T_index,T_begin,piece_data);
        //2.get mutex
        std::unique_lock<bthread::Mutex> mutex_guard(PeerManager::GetInstance()->GetSocketMutex(T_data->socket));
		send(T_socket,piece.GetSendData(),piece.GetDataSize(),0);
        return ;
    }
    
    //FD_SET socket
	void SelectSwitch::DPiece(Parameter* T_data)
	{
		if (nullptr== T_data)
			return ;
		std::unique_ptr<Parameter> guard(T_data);
        if (SLICESIZE!= (T_data->cur_len- 8))
        {
            Csz::ErrMsg("Select Switch recv piece is error,slice !=%d",SLICESIZE);
            return ;
        }
        //network byte
        int32_t index= ntohl(*reinterpret_cast<int32_t*>(T_data->buf));
        int32_t begin= ntohl(*reinterpret_cast<int32_t*>(T_data->buf+ 4));
        int32_t length= T_data->cur_len- 8;
        auto memory_pool= BitMemory::GetInstance();
        memory_pool->Write(index,begin,T_data->buf+ 8,length);
        //lock piece
        auto local_bit_field= LocalBitField::GetInstance();
        auto peer= NeedPiece::GetInstance();
        std::vector<uint8_t> over(TorrentFile::GetInstance()->GetPieceBit(index), 0);
        over[begin/SLICESIZE]= 1;
        int cur_socket= T_data->socket;
        while (!local_bit_field->CheckBitField(index))
        {
            
            for (auto& val : over)
            {
            }
        }
        return ;
	}
    
    inline bool SelectSwitch::_LockPiece(int T_socket,int32_t T_index,int32_t T_begin)
    {
        Request request;
        request.SetParameter(T_index,T_begin,SLICESIZE);
        {
            std::unique_lock<bthread::Mutex> mutex_guard(PeerManager::GetInstance()->GetSocketMutex(T_socket));
            if (send(T_socket,request.GetSendData(),request.GetDataSize(),0)!= SLICESIZE)
            {
                Csz::ErrMsg("Select Switch lock piece failed,send size!=%d",SLICESIZE)
                return false;
            }
        }
        int32_t len;
        uint8_t id;
        int32_t index;
        int32_t begin;
        char* block;
            
    }       
}
