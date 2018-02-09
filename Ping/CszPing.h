#ifndef CszPING_H
#define CszPING_H

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include  <netinet/ip_icmp.h>
#include <cstdlib> //calloc
#include <unistd.h> //setuid,alarm
#include <sys/socket.h> //setsockopt,recvmsg
#include <sys/time.h> //gettimeofday
#include <cstring> //memset
#include <signal.h> //SIGALRM
#include "../Sock/CszSocket.h" //CreateSocket,SocketNtopHost
#ifdef AF_INET6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif

#define PINGBUFSIZE 1500
//#define CszTest
#ifdef CszTest
#include "../Ip/CszDebugIp.h"
#endif
namespace Csz
{
	void InitV6();
	void ProcV4(char*,ssize_t,struct msghdr*,struct timeval*);
	void ProcV6(char*,ssize_t,struct msghdr*,struct timeval*);
	void SendV4();
	void SendV6();
	void ReadLoop();
	void SigAlrm(int);
	void TvSub(struct timeval*,struct timeval*);
	uint16_t InCksum(uint16_t*,int);
	struct Proto
	{
		void (*func_proc)(char*,ssize_t,struct msghdr*,struct timeval*);
		void (*func_send)();
		void (*func_init)();
		struct sockaddr* sa_send;
		struct sockaddr* sa_recv;
		socklen_t sa_len;
		int icmp_proto;
	};
}

#endif
