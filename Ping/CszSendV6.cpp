#include "CszPing.h"

extern char Send_buf[PINGBUFSIZE]; //main.cpp
extern int Pid; //main.cpp
extern int Data_len; //main.cpp
extern Csz::Proto* Pr; //main.cpp
extern int Nsend; //main.cpp
extern int Socket_fd; //CszReadLoop.cpp
namespace Csz
{
	void SendV6()
	{
#ifdef AF_INET6
		struct icmp6_hdr* icmp_v6;
		icmp_v6= (struct icmp6_hdr*)Send_buf;
		icmp_v6->icmp6_type= ICMP6_ECHO_REQUEST;
		icmp_v6->icmp6_code= 0;
		icmp_v6->icmp6_id= Pid;
		icmp_v6->icmp6_seq= Nsend++;
		memset((icmp_v6+ 1),0xa5,Data_len);
		gettimeofday((struct timeval*)(icmp_v6+ 1),NULL);
		int total=8+ Data_len;
		sendto(Socket_fd,Send_buf,total,0,Pr->sa_send,Pr->sa_len);
#endif
		return ;
	}
}
