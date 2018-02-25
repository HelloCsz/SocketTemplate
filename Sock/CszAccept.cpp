#include "CszSocket.h"

namespace Csz
{
	int Accept(int T_socket,struct sockaddr* T_addr,socklen_t* T_addrlen)
	{

		int socket;
again:
		//pending connections
		if ((socket= accept(T_socket,T_addr,T_addrlen))< 0)
		{
#ifdef EPROTO 
			if (EPROTO== errno || ECONNABORTED== errno)
#else
			if (ECONNABORTED== errno)
#endif
			goto again;

		}
		return socket;
	}
}
