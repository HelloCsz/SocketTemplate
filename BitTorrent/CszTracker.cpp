#include "CszBitTorrent.h"

namespace Csz
{
	Tracker::Tracker(){}

	Tracker::~Tracker()
	{
		for (const auto& val : info)
		{
			if (val.socket_fd>= 0)
				close(val.socket_fd);
		}
#ifdef CszTest
		printf("destructor Tracker\n");
#endif
		return ;
	}

	std::vector<int> Tracker::RetSocket()const
	{
		std::vector<int> ret;
		ret.reserve(info.size());
		for (const auto& val : info)
		{
			if (val.socket_fd>= 0)
			{
				ret.push_back(val.socket_fd);
			}
		}
#ifdef CszTest
		printf("Tracker have socket:\n");
		for (const auto& val : ret)
			printf("socket=%d\n",val);
#endif
		return std::move(ret);
	}

	void Tracker::SetTrackInfo(TrackerInfo T_data)
	{
		info.push_back(std::move(T_data));
		return ;
	}

	void Tracker::SetInfoHash(std::string T_data)
	{
		info_hash.assign(std::move(T_data));
		return ;
	}

	void Tracker::Connect()
	{
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
						Csz::ErrMsg("Tracker can't connect,nonblocking connect error");
						Csz::ErrMsg("host:%s,serv:%s\n",val.host.c_str(),val.serv.c_str());
						close(socket_fd);
						//释放空间再结束当前任务
						freeaddrinfo(res);
						continue;
					}
				}
				//恢复socket之前属性
				Fcntl(socket_fd,F_SETFL,flag);
				val.socket_fd= socket_fd;
#ifdef CszTest
				printf("Tcp connected %s\n",res->ai_canonname);
#endif
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
#ifdef CszTest
				printf("Udp connected %s\n",res->ai_canonname);
#endif
				freeaddrinfo(res);
			}
			else
			{
				Csz::ErrMsg("Tracker can't connect server,socket type is unknow,%s,fd=%d\n",val.host.c_str(),val.socket_fd);
				//val.socket_fd= -1;
			}
		}
		return ;
	}

	void Tracker::SetParameter(std::string T_data)
	{
		parameter_msg.append(1,'&');
		parameter_msg.append(std::move(T_data));
		return ;
	}

	std::vector<std::string> Tracker::GetPeerList(int T_sec)
	{
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
#ifdef CszTest
		printf("Tracker GetPeerList quit num:%d,fd_max:%d\n",quit_num,fd_max);
#endif
		wset_save= rset_save= wset= rset;
		//int count= 0;
		while (quit_num> 0)
		{
#ifdef CszTest
			printf("now quit num:%d\n",quit_num);
#endif
			//因为使用到一块共享内存作为接收缓冲(用来每次读取一行),可能发生interesting happend
			//从程序接收缓冲从读取到共享内存中,而这些数据包含着下次的数据,当再次调用select时,检查
			//的是程序接收缓冲区而不是共享内存区
			if (0== (/*count=*/ select(fd_max+ 1,&rset,&wset,NULL,T_sec? &time_val : NULL)))
			{
				//time out
				errno= ETIMEDOUT;
				Csz::ErrMsg("Tracker get peer list time out");
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
						Csz::ErrRet("Tracker can't get peer list,host:%s",val.host.c_str());
						close(val.socket_fd);
						val.socket_fd= -1;
						--quit_num;
						//--count;
						continue;
					}
					//移植问题 getsockopt return 0 if error
					else if (errno_save)
					{
						Csz::ErrMsg("Tracker can't get peer list,host:%s,%s",val.host.c_str(),strerror(errno_save));
						close(val.socket_fd);
						val.socket_fd= -1;
						--quit_num;
						//--count;
						continue;
					}
					//normal socket
					_Delivery(val.socket_fd,val.uri);
#ifdef CszTest
					printf("Delivery host:%s,serv:%s,%d\n",val.host.c_str(),val.serv.c_str(),val.socket_fd);
#endif
					//--count;
				}
				if (FD_ISSET(val.socket_fd,&rset))
				{
					FD_CLR(val.socket_fd,&rset_save);
#ifdef CszTest
					printf("Capturer host:%s,serv:%s,%d\n",val.host.c_str(),val.serv.c_str(),val.socket_fd);
#endif
					_Capturer(val.socket_fd);
                    ret_str.emplace_back(std::move(response->GetBody()));
					--quit_num;
				}
			}
			rset= rset_save;
			wset= wset_save;
		}
		return std::move(ret_str);
	}

	inline void Tracker::_Delivery(const int T_socket,const std::string& T_uri)
	{
		std::string request_line;
		request_line.reserve(128);
		request_line.append("GET ");
		request_line.append(T_uri);
		request_line.append(1,'?');
		request_line.append("info_hash=");
		request_line.append(info_hash);
		request_line.append(1,'&');
		request_line.append(parameter_msg);
		request_line.append(" HTTP/1.1");
		T_request->SetFirstLine(std::move(request_line));
#ifdef CszTest
		printf("Delivery Info:\n");
		T_request->COutInfo();
#endif
		struct iovec data_array[16];
		//hearder 与 body间隔\r\n\r\n size()+ 1
		size_t msg_num= T_request->size()>= 16? 16 : T_request->size()+ 1;
		if (!T_request->BindMsg(data_array,msg_num))
		{
			Csz::ErrMsg("Tracker can't Delivery");
			return ;
		}
		int send_num= writev(T_socket,data_array,msg_num);
#ifdef CszTest
		if (send_num< 0)
			printf("Tracker Delivery error:%s",strerror(errno));
		else
			printf("Delivery msg size:%d\n",send_num);
#endif
		T_request->ClearMethod();
		return ;
	}

	inline void Tracker::_Capturer(const int T_socket)
	{
        T_response->Clear();
		T_response->Capturer(T_socket_fd,T_cache);
		return ;
	}

#ifdef CszTest
	void Tracker::COutInfo()
	{
		printf("info hash:%s\n",info_hash.c_str());
		for (const auto& val : info)
		{
			printf("host:%s,serv:%s,uri:%s\n",val.host.c_str(),val.serv.c_str(),val.uri.c_str());
		}
		return ;
	}
#endif
}
