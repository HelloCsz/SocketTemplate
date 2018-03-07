#include <strings.h> //bzero
#include <sys/socket.h> //getsockopt,setsockopt,send,recv
#include <sys/select.h> //select
#include "CszBitTorrent.h"
#include "../Sock/CszSocket.h"

namespace Csz
{
/*
    PeerManager::PeerManager(const std::vector<std::string>& T_socket_list)
    {
        LoadPeerList(T_socket_list);   
    }
*/
    std::vector<int> PeerManager::RetSocketList() const
    {
        std::vector<int> ret;
        ret.reserve(peer_list.size());
        for (auto& val : peer_list)
        {
            if (val.first>= 0)
                ret.emplace_back(val.first);
        }
        return std::move(ret);
    }   

    void PeerManager::AddSocket(const int T_socket)
    {
        if (T_socket>= 0)
        {
            std::shared_ptr<bthread::Mutex> mutex(new bthread::Mutex());
            peer_list.emplace(std::make_pair(T_socket,mutex));
            DownSpeed::GetInstance()->AddSocket(T_socket);
        }
    }

    void PeerManager::LoadPeerList(const std::vector<std::string>& T_socket_list)
    {
        if (T_socket_list.empty())
        { 
            Csz::ErrMsg("Peer list is empty");
            return ;  
        }
        for (const auto& val : T_socket_list)
        {
            _LoadPeerList(val);
        }
        return ;
    }

    void PeerManager::_LoadPeerList(const std::string& T_socket_list)
    {
		std::vector<int> ret;
        auto flag= T_socket_list.find("peers");
        if (std::string::npos== flag)
        {
            return ;
        }
        flag+= 5;
        //1.load length num
        //throw invalid_argument or out_of_range
        size_t num_length;
        int num= std::stoi(std::string(T_socket_list.begin()+ flag,T_socket_list.begin()+ flag+ 3),&num_length);
        //peersxxx:
        auto start= T_socket_list.c_str()+ flag+ num_length+ 1;
        auto stop= T_socket_list.c_str()+ T_socket_list.size();
        //TODO 2.check comopact??
        while (num> 0 && start< stop)
        {
            sockaddr_in addr;
            bzero(&addr,sizeof(addr));
            addr.sin_family= AF_INET;
            addr.sin_addr.s_addr= *(reinterpret_cast<int32_t*>(const_cast<char*>(start)));
            addr.sin_port= *(reinterpret_cast<int16_t*>(const_cast<char*>(start+ 4)));
            start= start+ 6;
            //tcp  connect,only support ipv4
            int socket= Csz::CreateSocket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            //3.set nonblock
            auto old_flag= Csz::Fcntl(socket,F_GETFL,0);
            Csz::Fcntl(socket,F_SETFL,old_flag | O_NONBLOCK);
            if (connect(socket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))< 0)
            {
                if (errno!= EINPROGRESS)
                {
                    Csz::ErrMsg("PeerManager can't connect peer,nonblocking connection error");
                    Csz::ErrMsg("addr=%d,port=%d",ntohl(addr.sin_addr.s_addr),ntohs(addr.sin_port));
                    close(socket);
                    continue;
                }
            }
            //4.set old_flag
            Fcntl(socket,F_SETFL,old_flag);
            ret.emplace_back(socket);
        }
		//5.ensure connect and send hand shake
		_Connected(ret);
		//6.recv hand shake and delete failed socket
		_Verification(ret);
		//7.send bit field
		_SendBitField(ret);
		//TODO 8.lock and update peer list and socket_map_id
		NeedPiece::GetInstance()->SocketMapId(ret);
        auto down_speed= DownSpeed::GetInstance();
        for (const auto& val : ret)
        {
#ifdef CszTest
            if (peer_list.find(val)!= peer_list.end())
            {
                Csz::ErrMsg("Peer Manager peer list insert already exist");
                continue;  
            }
#endif
            std::shared_ptr<bthread::Mutex> mutex(new bthread::Mutex);
            peer_list.emplace(val,mutex);
            down_speed->AddSocket(val);
        }
        return ;       
    }
    
    PeerManager::~PeerManager()
    {
#ifdef CszTest
        Csz::LI("destructor Peer Manager");
#endif
        for (auto& val : peer_list)
        {
            if (val.first>= 0)
            {
                Csz::Close(val.first);
            }
        }
    }

    void PeerManager::_Connected(std::vector<int>& T_ret)
    {
		if (T_ret.empty())
			return ;
        //1.init select
        fd_set wset,rset;
        fd_set wset_save,rset_save;
        int fd_max= -1;
        
        //1.time out
        struct timeval time_val;
        time_val.tv_sec= 60;
        time_val.tv_usec= 0;  
        
        FD_ZERO(&wset);
        for (const auto& val : T_ret)
        {
            if (val>= 0)
            {
                if (fd_max< val)
                {
                    fd_max= val;
                }
                FD_SET(val,&wset);
            }
        }
		int stop= T_ret.size();
        rset_save= rset= wset_save= wset;
        //3.wait socket change able read or write
		int code;
        while (stop> 0)
        {
            if (0==(code=select(fd_max+ 1,&rset,&wset,NULL,&time_val)))
            {
                //time out
                errno= ETIMEDOUT;
                Csz::ErrMsg("wait peer time out");
                //TODO 继续循环
				for (auto& val : T_ret)
				{
					if (FD_ISSET(val,&rset_save) || FD_ISSET(val,&wset_save))
					{
						val= -1;
					}
				}
				//clear failed socket
				std::vector<int> ret;
				for (const auto& val : T_ret)
				{
					if (val >= 0)
					{
						ret.emplace_back(val);
					}
				}
				T_ret.swap(ret);
                return ;
            }
			if (code< 0)
			{
				Csz::ErrSys("select can't used");
				return ;
			}
            for (auto& val : T_ret)
            {
                if (val< 0)
                    continue ;
                if (FD_ISSET(val,&wset) || FD_ISSET(val,&rset))
                {
					FD_CLR(val,&rset_save);
					FD_CLR(val,&wset_save);
					--stop;
                    int errno_save= 0;
                    socklen_t errno_len= sizeof(errno_save);
                    if (getsockopt(val,SOL_SOCKET,SO_ERROR,&errno_save,&errno_len)< 0)
                    {
                        Csz::ErrRet("PeerManager can't connect peer");
                        Csz::Close(val);
                        val= -1;
                        continue ;
                    }
                    //getsockopt return 0 if error
                    else if (errno_save)
                    {
                        //strerror non-sofathread
                        Csz::ErrMsg("PeerManager can't connect peer:%s",strerror(errno_save));
                        Csz::Close(val);
                        val= -1;
                        continue ;
                    }
					//TODO auto
                    //nonmal socket
					auto hand_shake= HandShake::GetInstance();
					send(val,hand_shake->GetSendData(),hand_shake->GetDataSize(),0);
                }
            }
            rset= rset_save;
            wset= wset_save;
        }
		//clear failed socket
		std::vector<int> ret;
		for (const auto& val : T_ret)
		{
			if (val >= 0)
			{
				ret.emplace_back(val);
			}
		}
		T_ret.swap(ret);
        return ;
    }

	void PeerManager::_Verification(std::vector<int>& T_ret)
	{
		if (T_ret.empty())
			return ;
		auto hand_shake= HandShake::GetInstance();
        //1.init select
        fd_set rset,rset_save;
        int fd_max= -1;
        
        //1.time out
        struct timeval time_val;
        time_val.tv_sec= 60;
        time_val.tv_usec= 0;  
        
        FD_ZERO(&rset_save);
        for (const auto& val : T_ret)
        {
            if (val>= 0)
            {
				//set horizontal mark
				int lowat= 68;
				setsockopt(val,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
                if (fd_max< val)
                {
                    fd_max= val;
                }
                FD_SET(val,&rset_save);
            }
        }
		int stop= T_ret.size();
        rset= rset_save;
        //3.wait socket change able read
		int code;
        while (stop> 0)
        {
            if (0==(code=select(fd_max+ 1,&rset,NULL,NULL,&time_val)))
            {
                //time out
                errno= ETIMEDOUT;
                Csz::ErrMsg("wait peer time out");
                //TODO 继续循环
				for (auto& val : T_ret)
				{
					if (FD_ISSET(val,&rset_save))
					{
						val= -1;
					}
				}
				//clear failed socket
				std::vector<int> ret;
				for (const auto& val : T_ret)
				{
					if (val >= 0)
					{
						ret.emplace_back(val);
					}
				}
				T_ret.swap(ret);
                return ;
            }
			if (code< 0)
			{
				Csz::ErrSys("select can't used");
				return ;
			}
            for (auto& val : T_ret)
            {
                if (val< 0)
                    continue ;
                if (FD_ISSET(val,&rset))
                {
					FD_CLR(val,&rset_save);
					--stop;
					//TODO auto
					char buf[69]={0};
					if (68!=recv(val,buf,68,MSG_WAITALL))
					{
						val= -1;
					}
					else if (!(hand_shake->Varification(buf)))
					{
						val= -1;
					}
                }
            }
            rset= rset_save;
        }
		//clear failed socket
		std::vector<int> ret;
		for (const auto& val : T_ret)
		{
			if (val >= 0)
			{
				//set horizontal mark
				int lowat= 1;
				setsockopt(val,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
				ret.emplace_back(val);
			}
		}
		T_ret.swap(ret);
        return ;
	}

	void PeerManager::_SendBitField(std::vector<int>& T_ret)
	{
		if (T_ret.empty())
			return ;
		auto local_bit_field= LocalBitField::GetInstance();
		for (const auto& val : T_ret)
		{
			send(val,local_bit_field->GetSendData(),local_bit_field->GetDataSize(),0);
		}
		return ;
	}

	void PeerManager::CloseSocket(int T_socket)
	{
		auto result= peer_list.find(T_socket);
		if (result!= peer_list.end())
		{
			Csz::Close(T_socket);
			//lock
			peer_list.erase(result);
            NeedPiece::GetInstance()->ClearSocket(T_socket);
            DownSpeed::GetInstance()->ClearSocket(T_socket);
		}
		return ;
	}

    void PeerManager::SendHave(int32_t T_index)
    {
        if (T_index< 0)
        {
            Csz::ErrMsg("Peer Manager send have failed,index< 0");
            return ;
        }
        std::vector<decltype(peer_list)::const_iterator> del_sockets;
        Have have;
        have.SetParameter(T_index);
        int code;
        for (auto start= peer_list.cbegin(),stop= peer_list.cend(); start!= stop; ++start)
        {
            std::unique_lock<bthread::Mutex> guard(*(start->second));
            code= send(start->first,have.GetSendData(),have.GetDataSize(),0);
            if (-1== code || code!= have.GetDataSize())
            {
                del_sockets.emplace_back(start);
            }      
        }
        //TODO lock peer_list 
        for (const auto & val : del_sockets)
        {
            peer_list.erase(val->first);
        }
        return ;
    }
    
    //TODO safe
    bthread::Mutex* PeerManager::GetSocketMutex(int T_socket)
    {
        auto flag= peer_list.find(T_socket);
        if (flag== peer_list.end())
        {
            return nullptr;
        }
        return flag->second.get();
    } 
   
	void PeerManager::COutInfo()
	{
		std::string out_info;
		out_info.reserve(64);
		out_info.append("Peer Manager info:");
		for (auto &val : peer_list)
		{
			out_info.append("->"+std::to_string(val.first));
		}
		Csz::LI("%s",out_info.c_str());
		return ;
	}
}
