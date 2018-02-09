#include "CszPing.h"

extern int Verbose; //main.cpp
extern int Socket_fd; //CszReadLoop.cpp

namespace Csz
{
	void InitV6()
	{
#ifdef AF_INET6
		int on= 1;
		if (0== Verbose)
		{
			//install a fillter that only passes ICMP6_ECHO_REPLY unless verbose
			struct icmp6_filter my_filter;
			ICMP6_FILTER_SETBLOCKALL(&my_filter);
			ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY,&my_filter);
			setsockopt(Socket_fd,IPPROTO_IPV6,ICMP6_FILTER,&my_filter,sizeof(my_filter));
			//ignore error return,the filter is an optimization
		}
#ifdef IPV6_RECVHOPLIMIT
		setsockopt(Socket_fd,IPPROTO_IPV6,IPV6_RECVHOPLIMIT,&on,sizeof(on));
#else
		setsockopt(Socket_fd,IPPROTO_IPV6,IPV6_HOPLIMIT,&on,sizeof(on));
#endif
#endif
	}
}
