#include "CszSocket.h"

namespace Csz
{
	int Listen(int T_socket,int T_backlog)
	{
		if (listen(T_socket,T_backlog)< 0)
		{
			Csz::ErrSys("listen failed");
			return -1;
		}
		return 0;
	}
}
