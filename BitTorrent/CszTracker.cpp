#include <sys/uio.h> //writev
#include <sys/types.h>
#include <sys/socket.h> //connect
#include <netdb.h> //freeaddrinfo
#include <sys/select.h> //select
#include <time.h> //time
#include <stdlib.h> //rand,rand
#include "CszBitTorrent.h"
#include "../Sock/CszSocket.h" //Close
#include "../Web/CszWeb.h" //CszUrlEscape

namespace Csz
{
	Tracker::Tracker()
    {
#ifdef CszTest
		Csz::LI("construct Tracker");
#endif
        std::string parameter;
		auto id1= time(NULL);
		auto id2= std::rand()%100000000+ 1000000000;
		std::string id(std::to_string(id1));
		id.append(std::to_string(id2));
		parameter.append(Csz::UrlEscape()(id));
        am_id.assign(std::move(parameter));
        //init http request header
        _InitReq();
        //init http parameter msg
        _UpdateReq();
    }

	Tracker::~Tracker()
	{
		for (const auto& val : info)
		{
			if (val.socket_fd>= 0)
				close(val.socket_fd);
		}
#ifdef CszTest
		Csz::LI("destructor Tracker");
#endif
		return ;
	}

	std::vector<int> Tracker::RetSocket()const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::vector<int> ret;
		ret.reserve(info.size());
		for (const auto& val : info)
		{
			if (val.socket_fd>= 0)
			{
				ret.push_back(val.socket_fd);
			}
		}
		return std::move(ret);
	}

	void Tracker::SetTrackInfo(TrackerInfo T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		info.push_back(std::move(T_data));
		return ;
	}

	void Tracker::SetInfoHash(std::string T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		info_hash.assign(std::move(T_data));
		return ;
	}

	void Tracker::Connect()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		struct addrinfo* res;
		int socket_fd,flag;
		for (auto& val : info)
		{
			//tcp set -1,udp set -2,other unkonw
			if (-1== val.socket_fd)
			{
				//设置为地址族未定义 ipv4 or ipv6
				//释放res
				res= SocketHostServNull(val.host.c_str(),val.serv.c_str(),AF_UNSPEC,SOCK_STREAM);
				if (NULL== res)
					continue;
				socket_fd= CreateSocket(res->ai_family,res->ai_socktype,res->ai_protocol);
				//设置socket为非阻塞形式
				flag= Fcntl(socket_fd,F_GETFL,0);
				Fcntl(socket_fd,F_SETFL, flag | O_NONBLOCK);
				if (connect(socket_fd,res->ai_addr,res->ai_addrlen)< 0)
				{
					//如果不是in progress则退出程序并返回错误
					if (errno!= EINPROGRESS)
					{
						Csz::ErrMsg("[Tracker connect]->failed,can't connect,nonblocking connect error host:%s,serv:%s",val.host.c_str(),val.serv.c_str());
						close(socket_fd);
						//释放空间再结束当前任务
						freeaddrinfo(res);
						continue;
					}
				}
				//恢复socket之前属性
				Fcntl(socket_fd,F_SETFL,flag);
				val.socket_fd= socket_fd;
				freeaddrinfo(res);
			}
			else if (-2== val.socket_fd)
			{
				res= SocketHostServNull(val.host.c_str(),val.serv.c_str(),AF_UNSPEC,SOCK_DGRAM);
				if (NULL== res)
					continue;
				socket_fd= CreateSocket(res->ai_family,res->ai_socktype,res->ai_protocol);
				connect(socket_fd,res->ai_addr,res->ai_addrlen);
				val.socket_fd= socket_fd;
				freeaddrinfo(res);
			}
			else
			{
				Csz::ErrMsg("[Tracker connect]->failed,can't connect server,socket type is unknow,%s,fd=%d\n",val.host.c_str(),val.socket_fd);
				//val.socket_fd= -1;
			}
		}
		return ;
	}

	void Tracker::SetParameter(std::string T_data)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		parameter_msg.append(1,'&');
		parameter_msg.append(std::move(T_data));
		return ;
	}

	std::vector<std::string> Tracker::GetPeerList(int T_sec)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::vector<std::string> ret_str;
		fd_set wset,rset,rset_save,wset_save;
		int fd_max= -1;
		struct timeval time_val;
		time_val.tv_sec= T_sec;
		time_val.tv_usec= 0;
		FD_ZERO(&rset);
		int quit_num= 0;
		for (const auto& val : info)
		{
			if (val.socket_fd>= 0)
			{
				if (fd_max< val.socket_fd)
					fd_max= val.socket_fd;
				FD_SET(val.socket_fd,&rset);
				++quit_num;
			}
		}
		wset_save= rset_save= wset= rset;
		//int count= 0;
		while (quit_num> 0)
		{
			//因为使用到一块共享内存作为接收缓冲(用来每次读取一行),可能发生interesting happend
			//从程序接收缓冲从读取到共享内存中,而这些数据包含着下次的数据,当再次调用select时,检查
			//的是程序接收缓冲区而不是共享内存区
			if (0== (/*count=*/ select(fd_max+ 1,&rset,&wset,NULL,T_sec? &time_val : NULL)))
			{
				//time out
				errno= ETIMEDOUT;
				Csz::ErrMsg("[Tracker get peer list]->failed,time out");
				return std::move(ret_str);
			}
			//即可读又可写不同环境计算值不同,有的只计1,有些计2
			for (auto& val :info)
			{
				if (val.socket_fd< 0)
					continue;
				//不能假设链接成功后仅仅只有可写(在中间过程中可能接收到来自对端的数据)
				//但Tracker是特例
				if (FD_ISSET(val.socket_fd,&wset) /*|| FD_ISSET(val->socket_fd,&rset)*/)
				{
					FD_CLR(val.socket_fd,&wset_save);
					int errno_save= 0;
					socklen_t errno_len= sizeof(errno_save);
					if (getsockopt(val.socket_fd,SOL_SOCKET,SO_ERROR,&errno_save,&errno_len)< 0)
					{
						Csz::ErrRet("[Tracker get peer list]->failed,can't get peer list,host:%s",val.host.c_str());
						Csz::Close(val.socket_fd);
						val.socket_fd= -1;
						--quit_num;
						//--count;
						continue;
					}
					//移植问题 getsockopt return 0 if error
					else if (errno_save)
					{
						Csz::ErrMsg("[Tracker get peer list]->failed,can't get peer list,host:%s,%s",val.host.c_str(),strerror(errno_save));
						Csz::Close(val.socket_fd);
						val.socket_fd= -1;
						--quit_num;
						//--count;
						continue;
					}
					//normal socket
					_Delivery(val.socket_fd,val.host,val.serv,val.uri);
					//--count;
				}
				if (FD_ISSET(val.socket_fd,&rset))
				{
					FD_CLR(val.socket_fd,&rset_save);
					_Capturer(val.socket_fd);
                    ret_str.emplace_back(std::move(response.GetBody()));
                    //auto body= response.GetBody();
                    //ret_str.emplace_back(std::move(body));
					--quit_num;
				}
			}
			rset= rset_save;
			wset= wset_save;
		}
#ifdef CszTest
        COutInfo();
/*
        for (const auto& val : ret_str)
        {
            Csz::LI("%s",val.c_str());
			//std::cout<<val<<"\n";
        }
*/
#endif
		return std::move(ret_str);
	}

	void Tracker::_Delivery(const int T_socket,const std::string& T_host,const std::string& T_serv,const std::string& T_uri)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		//request.SetHeader("Host",T_host+":"+T_serv);
		std::string request_line;
		request_line.reserve(128);
		request_line.append("GET ");
		request_line.append(T_uri);
		request_line.append(1,'?');
		request_line.append("info_hash=");
		request_line.append(info_hash);
        request_line.append("&peer_id=");
        request_line.append(am_id);
		request_line.append(parameter_msg);
		request_line.append(" HTTP/1.1");
		request.SetFirstLine(std::move(request_line));

		struct iovec data_array[16];
		//hearder 与 body间隔\r\n\r\n size()+ 1
		size_t msg_num= request.size()>= 16? 16 : request.size()+ 1;
		if (!request.BindMsg(data_array,msg_num))
		{
			Csz::ErrMsg("[Tracker delivery]->failed");
			return ;
		}
		int send_num= writev(T_socket,data_array,msg_num);
#ifdef CszTest
		request.COutInfo();
#endif
		return ;
	}

	inline void Tracker::_Capturer(const int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        response.Clear();
		response.Capturer(T_socket,&cache);
#ifdef CszTest
		response.COutInfo();
#endif
		return ;
	}
    
    inline void Tracker::_InitReq()
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
	    request.SetHeader("Connection","Keep-alive");
	    request.SetHeader("User-Agent","Super Max");
	    request.SetHeader("Accept","text/html");
        return ;
    }
    
    void Tracker::_UpdateReq()
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        parameter_msg.clear();
        std::string parameter;
		parameter.assign("port=54321");
		SetParameter(std::move(parameter));

		parameter.assign("compact=1");
		SetParameter(std::move(parameter));

		parameter.assign("uploaded=0");
		SetParameter(std::move(parameter));

		parameter.assign("downloaded=0");
		SetParameter(std::move(parameter));

        //TODO update local file
		parameter.assign("left=");
		parameter.append(std::to_string(TorrentFile::GetInstance()->GetFileTotal()));
		SetParameter(std::move(parameter));

		parameter.assign("event=started");
		SetParameter(std::move(parameter));

		parameter.assign("numwant=20");
		SetParameter(std::move(parameter));
        return ;
    }
    
	void Tracker::COutInfo()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::string out_info;
		out_info.reserve(64);
		out_info.append("[Tracker INFO]:");
		out_info.append("info hash:"+info_hash);
		for (const auto& val : info)
		{
			out_info.append("[host:"+val.host+"serv:"+val.serv+"uri:"+ val.uri+"]");
		}
		if (!out_info.empty())
			Csz::LI("%s",out_info.c_str());
		return ;
	}
}
