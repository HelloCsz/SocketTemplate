#include "CszSocket.h"

namespace Csz
{
	int Bind(int T_socket,const struct sockaddr* T_addr,socklen_t T_addrlen)
	{
		if (bind(T_socket,T_addr,T_addrlen)< 0)
		{
			Csz::ErrSys("bind failed");
			return -1;
		}
		return 0;
	}
}
