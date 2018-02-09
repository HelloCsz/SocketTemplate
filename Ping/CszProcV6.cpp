#include "CszPing.h"

extern int Pid; //main.cpp
extern int Verbose; //main.cpp
extern Csz::Proto* Pr; //main.cpp
namespace Csz
{
	//原始IPV6 socket不返回ipv6首部,以辅助数据形式接收icmpv6分组的跳限(setsockopt)
	void ProcV6(char* T_buf,ssize_t T_buf_len,struct msghdr* T_msg,struct timeval* T_tv_recv)
	{
#ifdef AF_INET6
		struct icmp6_hdr* icmp_v6= (struct icmp6_hdr*)T_buf;
		//malformed packet
		if (T_buf_len< 8)
			return ;
		if (ICMP6_ECHO_REPLY== icmp_v6->icmp6_type)
		{
			if (icmp_v6->icmp6_id!= Pid)
				return ;
			if (T_buf_len< 16)
				return ;
			//icmp6_hdr只有必要的首部结构,icmp6_hdr+ 1则指向了数据区
			struct timeval* tv_send= (struct timeval*)(icmp_v6+ 1);
			TvSub(T_tv_recv,tv_send);
			double rtt= T_tv_recv->tv_sec* 1000.0+ T_tv_recv->tv_usec/ 1000.0;
			int hlim= -1;
			struct cmsghdr* cmsg;
			for (cmsg= CMSG_FIRSTHDR(T_msg); cmsg!= NULL; cmsg= CMSG_NXTHDR(T_msg,cmsg))
			{
				if (IPPROTO_IPV6==cmsg->cmsg_level && IPV6_HOPLIMIT== cmsg->cmsg_type)
				{
					hlim= *(uint32_t* )CMSG_DATA(cmsg);
					break;
				}
			}
			printf("%ld bytes from %s: seq=%u,hlim= ",T_buf_len,Csz::SocketNtoPHost(Pr->sa_recv,Pr->sa_len),icmp_v6->icmp6_seq);
			if (-1== hlim)
				printf("???");
			else
				printf("%d",hlim);
			printf(",rtt=%.3f ms\n",rtt);
		}
		else if (Verbose)
		{
			printf("%ld bytes from %s: type= %d,code= %d\n",T_buf_len,Csz::SocketNtoPHost(Pr->sa_recv,Pr->sa_len),icmp_v6->icmp6_type,icmp_v6->icmp6_code);
		}
#endif
	}
}
