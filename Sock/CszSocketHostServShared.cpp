#include "CszSocket.h"

namespace Csz
{
#if __cplusplus >= 201103L
	//error
	std::shared_ptr<struct addrinfo> SocketHostServShared(const char* T_host,const char* T_serv,int T_family,int T_socket_type)
	{
		/*
		struct addrinfo hints;
		bzero(&hints,sizeof(hints));
		hints.ai_flags= AI_CANONNAME;
		hints.ai_family= T_family;
		hints.ai_socktype= T_socket_type;
		int errno=save;
		//getaddrinfo参数res是指针的指针,如果用shared_ptr.get()进入的化,shared_ptr所管理的
		//内存将被覆盖掉,导致内存泄漏.
		std::shared_ptr<struct addrinfo> res(new struct addrinfo,[](struct addrinfo* T_res)
				{
					if (T_res!= NULL)
						freeaddrinfo(T_res);
					delete T_res;
				});
		*/
#endif
	}
}
