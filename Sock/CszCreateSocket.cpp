#include "CszSocket.h"

namespace Csz
{
	int CreateSocket(int T_domain,int T_type,int T_protocol)
	{
		int socket_fd= socket(T_domain,T_type,T_protocol);
		if (-1== socket_fd)
			Csz::ErrSys("CreateSocket can't create socket,socket type:%d",T_type);
		return socket_fd;
	}
}
