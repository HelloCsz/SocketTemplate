#include "CszPing.h"

extern Csz::Proto* Pr;//main.cpp
extern int Verbose; //main.cpp
extern int Pid; //main.cpp
namespace Csz
{
	void ProcV4(char* T_buf,ssize_t T_buf_len,struct msghdr* T_msg,timeval* T_tv_recv)
	{
		int ip_head_len,icmp_len;
		double rtt;
		struct ip* ip_4;
		struct icmp* icmp_4;
		struct timeval* tv_send;
		ip_4=(struct ip*)T_buf;
		//ip 首部字段是以四个字节为单位
#ifdef CszTest
		printf("ProcV4:\n");
		Csz::EchoIp4(ip_4);
#endif
		ip_head_len= ip_4->ip_hl<< 2;
		if(ip_4->ip_p!= IPPROTO_ICMP)
			return ;
		icmp_4= (struct icmp*)(T_buf+ ip_head_len);
#ifdef CszTest
		Csz::EchoIcmp4(icmp_4);
#endif
		//icmp首部8字节
		if ((icmp_len= T_buf_len- ip_head_len)< 8)
			return ;
		//echo reply
		if (ICMP_ECHOREPLY== icmp_4->icmp_type)
		{
			if(icmp_4->icmp_id!= Pid)
				return ;
			//
			if (icmp_len< 16)
				return ;
			tv_send= (struct timeval*)icmp_4->icmp_data;
			TvSub(T_tv_recv,tv_send);
			rtt= T_tv_recv->tv_sec* 1000.0+ T_tv_recv->tv_usec/ 1000.0;
			printf("%d bytes from %s: seq=%u,Pid=%u,ttl=%d,rtt=%.3f ms\n",
					icmp_len,Csz::SocketNtoPHost(Pr->sa_recv,Pr->sa_len),
					icmp_4->icmp_seq,icmp_4->icmp_id,ip_4->ip_ttl,rtt);
		}
		else if (Verbose)
		{
			printf("%d bytes from %s: type= %d,code=%d\n",
					icmp_len,Csz::SocketNtoPHost(Pr->sa_recv,Pr->sa_len),
					icmp_4->icmp_type,icmp_4->icmp_code);
		}
	}
}
