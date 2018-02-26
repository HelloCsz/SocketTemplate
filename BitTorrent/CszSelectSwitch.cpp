#include "CszBitTorrent.h"

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
		std::unique_ptr<Parameter,std::function<void(Parameter*)>> guard(T_data,[](const Parameter* T_del)
		{
			//roll bakc
			//lock ?
			FD_SET(T_del->socket,rset_save);
			delete T_del;
		});
		//1.go on recv
		int code;
		for (int i= 0; i< 3 && len> 0; ++i)
		{
			code= recv(T_data->socket,T_data->buf+ cur_len,T_data->len,MSG_DONTWAIT);
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
				PeerManager::GetInstance()->CloseSocket(T_data->socket);
			}
			T_data->cur_len += code;
			T_data->len -= code;
		}
		//2.1success recv
		if (0== T_data->len)
		{
			//RAII,unique_ptr repeat free memory
			auto origin= guard.release();
			DBitField(origin);
		}
		//2.3 time out,give up socket len < 0
		else
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
		if (DownSpeed::GetInstance()->CheckSocket(T_data->socket))
		{

		}
	}
}
