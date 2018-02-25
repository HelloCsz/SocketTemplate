#include "CszSocket.h"

namespace Csz
{
	int Close(int T_socket)
	{
		return close(T_socket);
	}
}
