#include "CszSocket.h"

namespace Csz
{
	int Fcntl(int T_socket,int T_cmd,int T_flag)
	{
		int flag= fcntl(T_socket,T_cmd,T_flag);
		if (flag< 0)
			Csz::ErrSys("Fcntl error");
		return flag;
	}
}
