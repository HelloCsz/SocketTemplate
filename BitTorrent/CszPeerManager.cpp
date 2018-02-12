#include "CszBitTorrent.h"

namespace Csz
{
    PeerManager::PeerManager(const std::vector<std::string>& T_socket_list)
    {
        LoadPeerList(T_socket_list);   
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
            LoadPeerListImpl(val);
        }
        return ;
    }

    inline void PeerManager::LoadPeerListImpl(const std::string& T_socket_list)
    {
        auto flag= T_socket_list.find("peers");
        if (std::string::npos== flag)
        {
            return ;
        }
        flag+= 5;
        //1.load length num
        //throw invalid_argument or out_of_range
        size_t num_length;
        int num= std::stoi(std::string(T_socket_list.begin()+ flag,T_sock_list.begin()+ flag+ 3),&num_length);
        //peersxxx:
        auto start= T_socket_list.c_str()+ flag+ num_length+ 1;
        auto stop= T_socket_list.c_str()+ T_socket_list.size();
        //TODO 2.check comopact??
        for (num> 0 && start< stop)
        {
            sockaddr_in addr;
            bzer(&addr,sizeof(addr));
            addr.sin_family= AF_INET;
            addr.sin_addr= *(static_cast<int32_t*>(start));
            addr.sin_port= *(static_cast<int16_t*>(start+ 4));
            start= start+ 6;
            //tcp  connect,only support ipv4
            int socket= Csz::CreateSocket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            //3.set nonblock
            auto old_flag= Csz::Fcntl(socket,F_GETFL,0);
            Csz::Fcntl(socket,F_SETFL,old_flag | O_NONBLOCK);
            if (connect(socket,static_cast<sockaddr*>(&addr),sizeof(addr))< 0)
            {
                if (errno!= EINPROGRESS)
                {
                    Csz::ErrMSG("PeerManager can't connect peer,nonblocking connection error");
                    Csz::ErrMsg("addr=%d,port=%d",ntohl(addr.sin_addr),ntohs(addr.sin_port));
                    close(socket);
                    continue;
                }
            }
            //4.set old_flag
            Fcntl(socket,F_SETFL,old_flag);
            peer_list.emplace_back(socket);
        }
        return ;       
    }

    PeerManager::~PeerManager()
    {
        for (auto& val : peer_list)
        {
            if (val>= 0)
            {
                close(val);
                val= -1;
            }
        }
    }
    void PeerManager::Run()
    {
        //1.init select
        fd_set wset,rset;
        fd_set wset_save,rset_save;
        int fd_max= -1;
        
        //TODO 2.tracker interval
        struct timeval time_val;
        time_val.tv_sec= 0;
        time_val.tv_usec= 0;  
        
        FD_ZERO(&wset);
        for (const auto& val : peer_list)
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
        rset_save= rset= wset_save= wset;
        //3.wait socket change able read or write
        while (true)
        {
            if (0==(select(fd_max+ 1,&rset,&wset,time_val.tv_sec> 0? &time_val : NULL)))
            {
                //time out
                errno= ETIMEDOUT;
                Csz::ErrMsg("wait peer time out");
                //TODO 继续循环
                return ;
            }
            for (auto& val : peer_list)
            {
                if (val< 0)
                    continue;
                if (FD_ISSET(val,&wset)
                {
                    int errno_save= 0;
                    sockelen_t errno_len= sizeof(errno_save);
                    if (getsockopt(val,SOL_SOCKET,SO_ERROR,&errno_save,&errno_len)< 0)
                    {
                        Csz::ErrRet("PeerManager can't connect peer");
                        close(val);
                        val= -1;
                        continue;
                    }
                    //getsockopt return 0 if error
                    else if (errno_save)
                    {
                        //strerror non-sofathread
                        Csz::ErrMsg("PeerManager can't connect peer:%s",strerror(errno_save);
                        close(val);
                        val= -1;
                        continue;
                    }
                    //nonmal socket
                    
                }
                if (FD_ISSET(val,&rset)
                {
                    //
                }
            }
            rset= rset_save;
            wset= wset_save;
        }
        return ;
    }
}
