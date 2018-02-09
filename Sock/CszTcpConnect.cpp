#include "CszSocket.h"

namespace Csz
{
	int TcpConnect(const char* T_host,const char* T_serv)
	{
		struct addrinfo hints,*res;
		bzero(&hints,sizeof(hints));
		hints.ai_family= AF_UNSPEC;
		hints.ai_socktype= SOCK_STREAM;
		//gai_strerror
		int errno_code;
		if ((errno_code= getaddrinfo(T_host,T_serv,&hints,&res))!= 0)
		{
			Csz::ErrQuit("getaddrinfo can't get host %s,serv %s,error message %s",T_host,T_serv,gai_strerror(errno_code));
			return -1;
		}
		//mark freeaddrinfo
		struct addrinfo* res_save= res;
		int socket_fd;
		for (;res!= NULL; res= res->ai_next)
		{
			socket_fd= socket(res->ai_family,res->ai_socktype,res->ai_protocol);
			if (socket_fd< 0)
				continue;
			if (0== connect(socket_fd,res->ai_addr,res->ai_addrlen))
				break;
			//connect 失败后需要close,否则下次connect直接失败
			close(socket_fd);
		}
		if (NULL== res)
		{
			//取代直接退出程序
			socket_fd= -1;
			Csz::ErrMsg("TcpConnect can't connect host %s,serv %s",T_host,T_serv);
		}
		freeaddrinfo(res_save);
		return socket_fd;
	}
}
