#include "CszPing.h"

extern char Send_buf[PINGBUFSIZE]; //main.cpp
extern int Pid; //main.cpp
extern int Nsend; //main.cpp
extern int Data_len; //main.cpp
extern int Socket_fd; //CszReadLoop.cpp
extern Csz::Proto* Pr; //main.cpp
namespace Csz
{
	void SendV4()
	{
		struct icmp* icmp_4= (struct icmp*)Send_buf;
		icmp_4->icmp_type= ICMP_ECHO;
		icmp_4->icmp_code= 0;
		icmp_4->icmp_id= Pid;
		icmp_4->icmp_seq= Nsend++;
		memset(icmp_4->icmp_data,0xa5,Data_len);
		gettimeofday((struct timeval*)icmp_4->icmp_data,NULL);
		int icmp_total= 8+ Data_len;
		icmp_4->icmp_cksum= 0;
		icmp_4->icmp_cksum= InCksum((uint16_t*)icmp_4,icmp_total);
#ifdef CszTest
		Csz::LI("SendV4:\n");
		Csz::EchoIcmp4(icmp_4);
#endif
		sendto(Socket_fd,Send_buf,icmp_total,0,Pr->sa_send,Pr->sa_len);
	}
}
