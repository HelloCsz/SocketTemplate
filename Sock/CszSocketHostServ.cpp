#include "CszSocket.h"

namespace Csz
{
	struct addrinfo* SocketHostServ(const char* T_host,const char* T_serv,int T_family,int T_sock_type)
	{
		struct addrinfo hints,*res= NULL;
		bzero(&hints,sizeof(hints));
		hints.ai_flags= AI_CANONNAME;
		hints.ai_family= T_family;
		hints.ai_socktype= T_sock_type;
		int errno_save;
		// success return 0
		if ((errno_save= getaddrinfo(T_host,T_serv,&hints,&res))!= 0)
		{
			Csz::ErrQuit("HostServ error for %s,%s,%s",
					(NULL== T_host)? "no hostname" : T_host,
					(NULL== T_serv)? "no service" : T_serv,
					gai_strerror(errno_save));
		}
		//记得释放
		return res;
	}
	struct addrinfo* SocketHostServNull(const char* T_host,const char* T_serv,int T_family,int T_sock_type)
	{
		struct addrinfo hints,*res= NULL;
		bzero(&hints,sizeof(hints));
		hints.ai_flags= AI_CANONNAME;
		hints.ai_family= T_family;
		hints.ai_socktype= T_sock_type;
		int errno_save;
		// success return 0
		if ((errno_save= getaddrinfo(T_host,T_serv,&hints,&res))!= 0)
		{
			Csz::ErrMsg("HostServ error for %s,%s,%s",
					(NULL== T_host)? "no hostname" : T_host,
					(NULL== T_serv)? "no service" : T_serv,
					gai_strerror(errno_save));
			return NULL;
		}
		//记得释放
		return res;
	}
}
