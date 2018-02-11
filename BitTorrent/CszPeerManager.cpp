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
        //TODO check comopact??
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
            //set nonblock
            auto tfl= Csz::Fcntl(socket,F_GETFL,0);
            Csz::Fcntl(socket,F_SETFL,tfl | O_NONBLOCK);
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
            //set old_flag
            Fcntl(socket,F_SETFL,tfl);
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
        fd_set wset        
    }
}
