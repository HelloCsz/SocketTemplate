#include <strings.h> //bzero
#include <sys/socket.h> //getsockopt,setsockopt,send,recv
#include <sys/select.h> //select
#include <butil/time.h> //gettimeofday_us
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
	
	//bug->data repetition
    std::vector<int> PeerManager::RetSocketList()
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::vector<int> ret;
        ret.reserve(peer_list.size());
        //TODO RAII lock
        //lock
        if (0!= pthread_rwlock_rdlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager ret socket list]->failed,read lock failed");
            return ret;
        }

#ifdef CszTest
        Csz::LI("[Peer Manager ret socket list]INFO:");
        COutInfo();
#endif
        for (auto& val : peer_list)
        {
            if (val.first>= 0 && 0==(((val.second)->status).recv_piece))
                ret.emplace_back(val.first);
        }
        pthread_rwlock_unlock(&lock);
        //unlock
#ifdef CszTest
		if (ret.empty())
			Csz::LI("[Peer Manager ret socket list]->failed,result is empty");
#endif
        return std::move(ret);
    }   

    void PeerManager::AddSocket(const int T_socket)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (T_socket>= 0)
        {
            std::shared_ptr<PeerManager::DataType> data= std::make_shared<PeerManager::DataType>();
            //lock
            if (0!= pthread_rwlock_wrlock(&lock))
            {
                Csz::ErrMsg("[Peer Manager add socket]->failed,write lock failed");
                return ;
            }
			data->id= cur_id++;
            auto temp_id= data->id;
            peer_list.emplace(std::make_pair(T_socket,data));
            pthread_rwlock_unlock(&lock);
			//unlock id
            DownSpeed::GetInstance()->AddSocket(T_socket);
			NeedPiece::GetInstance()->SocketMapId(temp_id);
        }
#ifdef CszTest
		else
		{
			Csz::LI("[Peer Manager add socket]->failed,socket< 0");
		}
#endif
		return ;
    }

    void PeerManager::LoadPeerList(const std::vector<std::string>& T_socket_list)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (T_socket_list.empty())
        { 
            Csz::ErrMsg("[Peer Manager load peer list]->failed,Peer list is empty");
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
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::vector<int> ret;
        auto flag= T_socket_list.find("peers");
        if (std::string::npos== flag)
        {
            Csz::ErrMsg("[Peer Manager load peer list]->failed,not found 'peers'");
            return ;
        }
        flag+= 5;
        //1.load length num
        //throw invalid_argument or out_of_range
        size_t num_length;
		int num= 0;
		try
		{
			num= std::stoi(std::string(T_socket_list.begin()+ flag,T_socket_list.begin()+ flag+ 3),&num_length);
		}
		catch (...)
		{
			Csz::ErrMsg("[Peer Manager load peer list]->failed,read num,line=%d",__LINE__);
			return ;
		}
        //peersxxx:
        auto start= T_socket_list.c_str()+ flag+ num_length+ 1;
        auto stop= start+ num;
        //TODO 2.check comopact??
        //TODO num-= 6
        while (num> 0 && start< stop)
        {
            sockaddr_in addr;
            bzero(&addr,sizeof(addr));
            addr.sin_family= AF_INET;
            addr.sin_addr.s_addr= *(reinterpret_cast<int32_t*>(const_cast<char*>(start)));
            addr.sin_port= *(reinterpret_cast<int16_t*>(const_cast<char*>(start+ 4)));
            num-= 6;
            start= start+ 6;
#ifdef CszTest
			//all thread,in only one thread run
            Csz::LI("ip:%s->port:%d",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
#endif
            //tcp  connect,only support ipv4
            int socket= Csz::CreateSocket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            //3.set nonblock
            auto old_flag= Csz::Fcntl(socket,F_GETFL,0);
            Csz::Fcntl(socket,F_SETFL,old_flag | O_NONBLOCK);
            if (connect(socket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))< 0)
            {
                if (errno!= EINPROGRESS)
                {
                    Csz::ErrMsg("[Peer Manager load peer list]->failed,can't connect peer,nonblocking connection error addr=%d,port=%d",ntohl(addr.sin_addr.s_addr),ntohs(addr.sin_port));
					Csz::Close(socket);
                    continue;
                }
            }
            //4.set old_flag
            Fcntl(socket,F_SETFL,old_flag);
            ret.emplace_back(socket);
        }
		//5.ensure connect and send hand shake
		ret= std::move(_Connected(std::move(ret)));
#ifdef CszTest
        Csz::LI("[Peer Manager load peer list]connected size=%d",ret.size());
#endif
		//6.recv hand shake and delete failed socket
		ret= std::move(_Verification(std::move(ret)));
#ifdef CszTest
        Csz::LI("[Peer Manager load peer list]verification size=%d",ret.size());
#endif
        //7. send bit field
		ret= std::move(_SendBitField(std::move(ret)));
#ifdef CszTest
        Csz::LI("[Peer Manager load peer list]send bit field size=%d",ret.size());
#endif
		//TODO 8.lock and update peer list and socket_map_id
		std::vector<int> socket_id;
		socket_id.reserve(ret.size());
		auto need_piece= NeedPiece::GetInstance();
        auto down_speed= DownSpeed::GetInstance();
        for (const auto& val : ret)
        {
			//TODO lock id
			if (0!= pthread_rwlock_wrlock(&lock))
            {
                Csz::ErrMsg("[Peer Manager peer list]->failed,write lock failed");
                continue ;
            }
#ifdef CszTest
            if (peer_list.find(val)!= peer_list.end())
            {
                Csz::ErrMsg("[Peer Manager peer list]->failed,insert already exist");
                continue;  
            }
#endif
            std::shared_ptr<PeerManager::DataType> data= std::make_shared<PeerManager::DataType>();
			data->id= cur_id++;
            data->mutex= std::make_shared<bthread::Mutex>();
            auto temp_id= data->id;
            peer_list.emplace(std::make_pair(val,data));
            pthread_rwlock_unlock(&lock);
			//unlock id

            down_speed->AddSocket(val);
			need_piece->SocketMapId(temp_id);

        }
        return ;       
    }
    
    PeerManager::~PeerManager()
    {
#ifdef CszTest
        Csz::LI("destructor Peer Manager");
#endif
        pthread_rwlock_destroy(&lock);
        for (auto& val : peer_list)
        {
            if (val.first>= 0)
            {
                Csz::Close(val.first);
            }
        }
    }

    std::vector<int> PeerManager::_Connected(std::vector<int> T_ret)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::vector<int> ret;
		if (T_ret.empty())
		{
            Csz::ErrMsg("[Peer Manager _connected]->failed line:%d,parameter socket is empty",__LINE__);
            return ret;
        }
        const int MAX_EVENTS= 25;
        struct epoll_event ev,events[MAX_EVENTS];
        auto epollfd= epoll_create(T_ret.size());
        if (-1== epollfd)
        {
            Csz::ErrRet("[Peer Manager _connected]->failed line:%d,epoll create,",__LINE__);
            return ret;
        }
        int quit_num= 0;
        for (const auto& val : T_ret)
        {
            if (val>= 0)
            {
                ev.events= EPOLLOUT;
                ev.data.fd= val;
                if (-1== epoll_ctl(epollfd,EPOLL_CTL_ADD,val,&ev))
                {
                    Csz::ErrRet("[Peer Manager _connected]->failed line:%d,epoll ctl,",__LINE__);
                }
                else
                {
                    ++quit_num;
                }
            }
        }
        //3.wait socket change able read or write
		int code;
        while (quit_num> 0)
        {
            if (0==(code= epoll_wait(epollfd,events,MAX_EVENTS,16 *1000)))
            {
                //time out
                errno= ETIMEDOUT;
                Csz::ErrMsg("[Peer Manager _connected]->failed line:%d,wait peer time out",__LINE__);
                //TODO 继续循环
                break ;
            }
			if (code< 0)
			{
				Csz::ErrSys("[Peer Manager _connected]->failed line:%d,epoll wait can't used",__LINE__);
				break ;
			}
            quit_num-= code;
            for (int n= 0; n< code; ++n)
            {
                int errno_save= 0;
                socklen_t errno_len= sizeof(errno_save);
                if (getsockopt(events[n].data.fd,SOL_SOCKET,SO_ERROR,&errno_save,&errno_len)< 0)
                {
                    Csz::ErrRet("[Peer Manager _connected]->failed line:%d,can't connect peer",__LINE__);
                    if (-1== epoll_ctl(epollfd,EPOLL_CTL_DEL,events[n].data.fd,NULL))
                    {
                        Csz::ErrRet("[Peer Manager _connected]->failed line:%d,",__LINE__);
                    }
                    Csz::Close(events[n].data.fd);
                    continue ;
                }
                //getsockopt return 0 if error
                else if (errno_save)
                {
                    //strerror non-sofathread
                    Csz::ErrMsg("[Peer Manager _connected]->failed line:%d,can't connect peer:%s",__LINE__,strerror(errno_save));
                    if (-1== epoll_ctl(epollfd,EPOLL_CTL_DEL,events[n].data.fd,NULL))
                    {
                        Csz::ErrRet("[Peer Manager _connected]->failed line:%d,",__LINE__);
                    }
                    Csz::Close(events[n].data.fd);
                    continue ;
                }
				//TODO auto
                //nonmal socket
			    auto hand_shake= HandShake::GetInstance();
				send(events[n].data.fd,hand_shake->GetSendData(),hand_shake->GetDataSize(),0);
                ret.push_back(events[n].data.fd);
                if (-1== epoll_ctl(epollfd,EPOLL_CTL_DEL,events[n].data.fd,NULL))
                {
                    Csz::ErrRet("[Peer Manager _connected]->failed line:%d,",__LINE__);
                }
            }
        }
        return std::move(ret);
    }

	std::vector<int> PeerManager::_Verification(std::vector<int> T_ret)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::vector<int> ret;
		if (T_ret.empty())
		{
            Csz::ErrMsg("[Peer Manager _verification]->failed line:%d,parameter socket is empty",__LINE__);
            return ret;
        }
		auto hand_shake= HandShake::GetInstance();
        const int MAX_EVENTS= 25;
        struct epoll_event ev,events[MAX_EVENTS];
        auto epollfd= epoll_create(T_ret.size());
        if (-1== epollfd)
        {
            Csz::ErrRet("[Peer Manager _verification]->failed line:%d,epoll create,",__LINE__);
            return ret;
        }
        int quit_num= 0;
        for (auto& val : T_ret)
        {
            if (val>= 0)
            {
                ev.events= EPOLLIN;
                ev.data.fd= val;
                if (-1== epoll_ctl(epollfd,EPOLL_CTL_ADD,val,&ev))
                {
                    Csz::ErrRet("[Peer Manager _verification]->failed line:%d,epoll ctl,",__LINE__);
                }
                else
                {
                    int lowat= 68;
                    setsockopt(val,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
                    ++quit_num;
                }
            }
        }
		int code;
        char buf[69]={0};
        while (quit_num> 0)
        {
            if (0==(code= epoll_wait(epollfd,events,MAX_EVENTS,16 *1000)))
            {
                //time out
                errno= ETIMEDOUT;
                Csz::ErrMsg("[Peer Manager _verification]->failed line:%d,wait peer time out",__LINE__);
                //TODO 继续循环
                break ;
            }
			if (code< 0)
			{
				Csz::ErrSys("[Peer Manager _verification]->failed line:%d,epoll wait can't used",__LINE__);
				break ;
			}
            quit_num-= code;
            for (int n= 0; n< code; ++n)
            {
                if (68!= recv(events[n].data.fd,buf,68,MSG_WAITALL))
                {
                    Csz::ErrMsg("[Peer Manager _verification]->failed,recv num!= 68");;
                    Csz::Close(events[n].data.fd);
                }
                else if (!(hand_shake->Varification(buf)))
                {
                    Csz::ErrMsg("[Peer Manager _verification]->failed,recv num!= 68");;
                    Csz::Close(events[n].data.fd);
                }
                else
                {
                    ret.push_back(events[n].data.fd);
                }
                if (-1== epoll_ctl(epollfd,EPOLL_CTL_DEL,events[n].data.fd,NULL))
                {
                    Csz::ErrRet("[Peer Manager _verification]->failed line:%d,",__LINE__);
                }
            }
        }
        return std::move(ret);
	}

	std::vector<int> PeerManager::_SendBitField(std::vector<int> T_ret)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::vector<int> ret;
		if (T_ret.empty())
	    {
            Csz::ErrMsg("[Peer Manager send bit field]->failed,parameter socket is empty");
        	return ret;
        }
        auto data= LocalBitField::GetInstance()->GetSendData();
        auto len= LocalBitField::GetInstance()->GetDataSize();
		for (const auto& val : T_ret)
		{
			if(send(val,data,len,0)== len)
            {
                ret.push_back(val);
            }
		}
		return std::move(ret);
	}

	void PeerManager::CloseSocket(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager close socket]->failed,write lock failed");
            return ;
        }
		auto result= peer_list.find(T_socket);
		if (result!= peer_list.end())
		{
			Csz::Close(result->first);
            auto temp_id= result->second->id;
			peer_list.erase(result);
            pthread_rwlock_unlock(&lock);
            //unlock

			//socket
            DownSpeed::GetInstance()->ClearSocket(T_socket);
			//id
            NeedPiece::GetInstance()->ClearSocket(temp_id);

		}
		else
		{
            pthread_rwlock_unlock(&lock);
            //unlock

			Csz::ErrMsg("[Peer Manager close socket]->failed,not found socket");
		}
		return ;
	}

    void PeerManager::SendHave(int32_t T_index)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (T_index< 0)
        {
            Csz::ErrMsg("[Peer Manager send have]->failed,index< 0");
            return ;
        }

        //bug ,iterator invalid,upon data after unlock delete than down on invalid
        std::vector<decltype(peer_list)::key_type> del_sockets;
        Have have;
        have.SetParameter(T_index);
        int code;

        //lock
        if (0!= pthread_rwlock_rdlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager send have]->failed,read lock failed");
            return ;
        }
        for (auto start= peer_list.cbegin(),stop= peer_list.cend(); start!= stop; ++start)
        {
            std::lock_guard<bthread::Mutex> guard(*(start->second->mutex));
            code= send(start->first,have.GetSendData(),have.GetDataSize(),0);
            if (-1== code || code != have.GetDataSize())
            {
                del_sockets.emplace_back(start->first);
            }      
        }
        pthread_rwlock_unlock(&lock);
        //unlock
        
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager send have]->failed,write lock failed");
            return ;
        }

        //TODO lock peer_list 
        for (const auto & val : del_sockets)
        {
            peer_list.erase(val);
        }
        pthread_rwlock_unlock(&lock);
        //unlock
        return ;
    }
    
    //TODO not safe,peer erase socket,but mutex out put
    std::shared_ptr<bthread::Mutex> PeerManager::GetSocketMutex(int T_socket)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        std::shared_ptr<bthread::Mutex> ret(nullptr);
        if (0!= pthread_rwlock_rdlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager get socket mutex]->failed,read lock failed");
            return ret;
        }   
        auto flag= peer_list.find(T_socket);
        if (flag!= peer_list.end())
        {
            ret= flag->second->mutex;
        }
        //unlock
        pthread_rwlock_unlock(&lock);
        return ret;
    } 
 
	int PeerManager::GetSocketId(int T_socket) 
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        int ret= -1;
        if (0!= pthread_rwlock_rdlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager get socket id]->failed,read lock failed");
            return ret;
        }   
		if (peer_list.find(T_socket)!= peer_list.end())
		{
			ret= peer_list[T_socket]->id;
		}
        pthread_rwlock_unlock(&lock);
		return ret;
	}

	void PeerManager::AmChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager am choke]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		if ((flag->second->status).am_choke)
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).am_choke= 1;
        auto mutex= flag->second->mutex;
        auto temp_id= flag->second->id;
        pthread_rwlock_unlock(&lock);
        //unlock
        
		NeedPiece::GetInstance()->AmChoke(temp_id);
		DownSpeed::GetInstance()->AmChoke(T_socket);
		Choke choke;
		std::lock_guard<bthread::Mutex> guard(*mutex);
		send(T_socket,choke.GetSendData(),choke.GetDataSize(),0);
		return ;
	}

	void PeerManager::AmUnChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager am unchoke]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		if (!(flag->second->status).am_choke)
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).am_choke= 0;
        auto temp_id= flag->second->id;
        auto mutex= flag->second->mutex;
        pthread_rwlock_unlock(&lock);
        //unlock

		NeedPiece::GetInstance()->AmUnChoke(temp_id);
		DownSpeed::GetInstance()->AmUnChoke(T_socket);
		UnChoke unchoke;
		std::lock_guard<bthread::Mutex> guard(*mutex);
		send(T_socket,unchoke.GetSendData(),unchoke.GetDataSize(),0);
		return ;
	}

	void PeerManager::AmInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager am interested]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		if ((flag->second->status).am_interested)
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).am_interested= 1;
        auto temp_id= flag->second->id;
        auto mutex= flag->second->mutex;
        pthread_rwlock_unlock(&lock);
        //unlock

		NeedPiece::GetInstance()->AmInterested(temp_id);
		DownSpeed::GetInstance()->AmInterested(T_socket);
		Interested interested;
		std::lock_guard<bthread::Mutex> guard(*mutex);
		send(T_socket,interested.GetSendData(),interested.GetDataSize(),0);
		return ;
	}

	void PeerManager::AmUnInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager am uninterested]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		if (!(flag->second->status).am_interested)
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).am_interested= 0;
        auto temp_id= flag->second->id;
        auto mutex= flag->second->mutex;
        pthread_rwlock_unlock(&lock);
        //unlock

		NeedPiece::GetInstance()->AmUnInterested(temp_id);
		DownSpeed::GetInstance()->AmUnInterested(T_socket);
		UnInterested uninterested;
		std::lock_guard<bthread::Mutex> guard(*mutex);
		send(T_socket,uninterested.GetSendData(),uninterested.GetDataSize(),0);
        return ;
	}

	void PeerManager::PrChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager pr choke]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).peer_choke= 1;
        auto temp_id= flag->second->id;
        pthread_rwlock_unlock(&lock);
        //unlock

		NeedPiece::GetInstance()->PrChoke(temp_id);
		DownSpeed::GetInstance()->PrChoke(T_socket);
		return ;
	}

	void PeerManager::PrUnChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager pr unchoke]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).peer_choke= 0;
        auto temp_id= flag->second->id;
        pthread_rwlock_unlock(&lock);
        //unlock
        
		NeedPiece::GetInstance()->PrUnChoke(temp_id);
		DownSpeed::GetInstance()->PrUnChoke(T_socket);
		return ;
	}

	void PeerManager::PrInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager pr interested]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).peer_interested= 1;
        auto temp_id= flag->second->id;
        pthread_rwlock_unlock(&lock);
        //unlock
        
		NeedPiece::GetInstance()->PrInterested(temp_id);
		DownSpeed::GetInstance()->PrInterested(T_socket);
		return ;
	}

	void PeerManager::PrUnInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager pr uninterested]->failed,write lock failed");
            return ;
        }   
		auto flag= peer_list.find(T_socket);
		if (flag== peer_list.end())
		{
            pthread_rwlock_unlock(&lock);
			return ;
		}
		(flag->second->status).peer_interested= 0;
        auto temp_id= flag->second->id;
        pthread_rwlock_unlock(&lock);
        //unlock
        
		NeedPiece::GetInstance()->PrUnInterested(temp_id);
		DownSpeed::GetInstance()->PrUnInterested(T_socket);
	}
	
	void PeerManager::Optimistic()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		//TODO lock
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Peer Manager pr choke]->failed,write lock failed");
            return ;
        }  
        //fix bug floating point exception
        int num= peer_list.size()- 4;
        if (num<= 0)
        {
            num= 1;  
        }
		int optimistic= butil::gettimeofday_us() % num;
		for (auto& val : peer_list)
		{
			if (((val.second)->status).am_choke)
			{
				--optimistic;
				if (optimistic<= 0)
				{
		            ((val.second)->status).am_choke= 0;
		            NeedPiece::GetInstance()->AmUnChoke((val.second)->id);
		            DownSpeed::GetInstance()->AmUnChoke(val.first);
		            UnChoke unchoke;
		            std::lock_guard<bthread::Mutex> guard(*((val.second)->mutex));
		            send(val.first,unchoke.GetSendData(),unchoke.GetDataSize(),0);
					break;
				}
			}
		}
        pthread_rwlock_unlock(&lock);
		return ;
	}

    bool PeerManager::ReqPiece(int T_socket)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        bool ret= false;
        if (T_socket< 0)
        {
            Csz::ErrMsg("[Peer Manager req piece]->failed,socket< 0");
            return ret;
        }
        if (pthread_rwlock_wrlock(&lock)!= 0)
        {
            Csz::ErrMsg("[Peer Manager req piece]->failed,write lock failed");
            return ret;
        }
        auto flag= peer_list.find(T_socket);
        if (flag!= peer_list.end())
        {
            if ((flag->second->status).request_piece)
            {
                if (!(flag->second->status).recv_piece)
                {
                    if (time(NULL)- flag->second->expire>= PEERPIECETIME)
                    {
                        ret= true;
                        flag->second->expire= time(NULL);
                    }
                }
            }
            else
            {
                ret= true;
                (flag->second->status).request_piece= 1;
                flag->second->expire= time(NULL);
            }
        }
        pthread_rwlock_unlock(&lock);
        return ret;
    }

    bool PeerManager::RecvPiece(int T_socket)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        bool ret= false;
        if (T_socket< 0)
        {
            Csz::ErrMsg("[Peer Manager recv piece]->failed,socket< 0");
            return ret;
        }
        if (pthread_rwlock_wrlock(&lock)!= 0)
        {
            Csz::ErrMsg("[Peer Manager recv piece]->failed,write lock failed");
            return ret;
        }
        auto flag= peer_list.find(T_socket);
        if (flag!= peer_list.end())
        {
            if (!(flag->second->status).recv_piece)
            {
                ret= true;
                (flag->second->status).recv_piece= 1;
            }
        }
        pthread_rwlock_unlock(&lock);
        return ret;
    }

    void PeerManager::ClearPieceStatus(int T_socket)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (T_socket< 0)
        {
            Csz::ErrMsg("[Peer Manager clear piece status]->failed,socket< 0");
            return ;
        }
        if (pthread_rwlock_wrlock(&lock)!= 0)
        {
            Csz::ErrMsg("[Peer Manager clear piece status]->failed,write lock failed");
            return ;
        }
        auto flag= peer_list.find(T_socket);
        if (flag!= peer_list.end())
        {
            flag->second->expire= 0;
            (flag->second->status).request_piece= 0;
            (flag->second->status).recv_piece= 0;
        }
        pthread_rwlock_unlock(&lock);
        return ;
    }

	void PeerManager::COutInfo() const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::string out_info;
		out_info.reserve(64);
		out_info.append("[Peer Manager INFO]:");
		for (auto &val : peer_list)
		{
			out_info.append("["+std::to_string(val.first)+":"+ std::to_string((val.second)->id)+"]");
		}
		if (!out_info.empty())
			Csz::LI("%s,size=%d",out_info.c_str(),peer_list.size());
		return ;
	}
}
