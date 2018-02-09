#include "CszSocket.h"

namespace Csz
{
	char* SocketNtoPHost(const struct sockaddr* T_sa,socklen_t T_sa_len)
	{
		//unix socket地址最大字节数
		static char buff_addr[128]={0};
		switch (T_sa->sa_family)
		{
			case AF_INET:
			{		
				struct sockaddr_in* temp= (sockaddr_in*)T_sa;
				if (NULL== inet_ntop(temp->sin_family,&temp->sin_addr,buff_addr,sizeof(buff_addr)))
				{
					return NULL;
				}
				return buff_addr;
			}
#ifdef AF_INET6
			case AF_INET6:
			{
				struct sockaddr_in6* temp= (sockaddr_in6*)T_sa;
				if (NULL== inet_ntop(temp->sin6_family,&temp->sin6_addr,buff_addr,sizeof(buff_addr)))
				{
					return NULL;
				}
				return buff_addr;
			}
#endif
			default:
			{
				snprintf(buff_addr,sizeof(buff_addr),"SockNtoPHost: unknow AF_XXX: %d,len:%d",T_sa->sa_family,T_sa_len);
				return buff_addr;
			}
		}
	}
}
