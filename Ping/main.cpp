#include <unistd.h> //getopt,getpid
#include "/home/hello-wxx/SocketTemplate/Error/CszError.h"
#include "/home/hello-wxx/SocketTemplate/Signal/CszSignal.h"
#include "/home/hello-wxx/SocketTemplate/Sock/CszSocket.h"
#include "CszPing.h"
Csz::Proto proto_v4={Csz::ProcV4,Csz::SendV4,NULL,NULL,NULL,0,IPPROTO_ICMP};
#ifdef AF_INET6
Csz::Proto proto_v6={Csz::ProcV6,Csz::SendV6,Csz::InitV6,NULL,NULL,0,IPPROTO_ICMPV6};
#endif

char Send_buf[PINGBUFSIZE]={0};
int Data_len= 56;
int Verbose= 0;
int Pid;
Csz::Proto* Pr;
int Nsend= 0;

void* Calloc(size_t T_num,size_t T_size)
{
	void* temp= calloc(T_num,T_size);
	return temp;
}

int main(int argc,char** argv)
{
	int ch;
	//set opterr=0,dont't want getopt writing to stderr
	opterr= 0;
	while ((ch= getopt(argc,argv,"v"))!= -1)
	{
		switch(ch)
		{
			case 'v':
				++Verbose;
				break;
			case '?':
				Csz::ErrQuit("unrecognized option:%c",ch);
				break;
		}
	}
	//-v参数后一个参数不是最后一个参数
	//若带-v参数 optind= 2
	if (optind!= argc- 1)
		Csz::ErrQuit("usage:ping[-v] <hostname>");
	const char* host= argv[optind];
	//后十六位
	Pid= getpid()& 0xffff;
	if (SIG_ERR== Csz::SignalAlarm(SIGALRM,Csz::SigAlrm))
	{
		Csz::ErrSys("can't run Signal,signal SIGALRM");
	}
	//释放addr_info
	struct addrinfo* addr_info= Csz::SocketHostServ(host,NULL,0,0);
	struct addrinfo* addr_info_save= addr_info;
	char* host_str= Csz::SocketNtoPHost(addr_info->ai_addr,addr_info->ai_addrlen);
	printf("Ping %s (%s) : %d data bytes\n",
			addr_info->ai_canonname== NULL? host_str : addr_info->ai_canonname,
			host_str,
			Data_len
			);
	if (AF_INET== addr_info->ai_family)
	{
		Pr= &proto_v4;
	}
#ifdef AF_INET6
	else if (AF_INET6== addr_info->ai_family)
	{
		Pr= &proto_v6;
		if (IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6*)addr_info->ai_addr)->sin6_addr)))
			Csz::ErrQuit("cannnot ping IPV4-mapped IPV6 address");
	}
#endif
	else
		Csz::ErrQuit("unknow address family %d",addr_info->ai_family);
	Pr->sa_send= addr_info->ai_addr;
	Pr->sa_recv=(struct sockaddr*) Calloc(1,addr_info->ai_addrlen);
	if (NULL== Pr->sa_recv)
	{
		freeaddrinfo(addr_info_save);
		Csz::ErrQuit("Calloc can't new storage");
	}
	Pr->sa_len= addr_info->ai_addrlen;
	Csz::ReadLoop();
	freeaddrinfo(addr_info_save);
	free(Pr->sa_recv);
	return 0;

}
