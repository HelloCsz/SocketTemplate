#include "CszSocket.h"

namespace Csz
{
	int TcpConnectTime(const char* T_host,const char* T_serv,int T_sec)
	{
		SigHandler sig_handler_old= Signal(SIGALRM,[](int){return;});
		if (alarm(T_sec)!= 0)
		{
			Csz::ErrMSg("TcpConnectTime:alarm was already set");
		}
		int socket_fd;
		if ((socket_fd= TcpConnect(T_host,T_serv))< 0)
		{
			close (socket_fd);
			if (errno== EINTR)
				errno= ETIMEDOUT;
		}
		alarm(0);
		Signal(SIGALRM,sig_handler_old);
		return socket_fd;
	}
}
