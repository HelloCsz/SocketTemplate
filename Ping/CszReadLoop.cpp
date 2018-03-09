#include "CszPing.h"

extern Csz::Proto* Pr; //main.cpp
int Socket_fd;

namespace Csz
{
	void ReadLoop()
	{
		int size;
		char recv_buf[PINGBUFSIZE]={0};
		char control_buf[PINGBUFSIZE]={0};
		struct msghdr msg;
		struct iovec iov;
		Socket_fd= Csz::CreateSocket(Pr->sa_send->sa_family,SOCK_RAW,Pr->icmp_proto);
		setuid(getuid());
		if (Pr->func_init)
			Pr->func_init();
		size= 60* 1024;
		setsockopt(Socket_fd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
		Csz::SigAlrm(SIGALRM);
		iov.iov_base= recv_buf;
		iov.iov_len= sizeof(recv_buf);
		msg.msg_name= Pr->sa_recv;
		msg.msg_iov= &iov;
		msg.msg_iovlen= 1;
		msg.msg_control= control_buf;
		ssize_t recv_num;
		struct timeval tval;
		for (int i= 0;i< 30;++i)
		{
			msg.msg_namelen= Pr->sa_len;
			msg.msg_controllen= sizeof(control_buf);
			recv_num= recvmsg(Socket_fd,&msg,0);
			if (recv_num< 0)
			{
#ifdef CszTest
				Csz::LI("INTR\n");
#endif
				if (EINTR== errno)
				{
					continue;
				}
				else
					Csz::ErrSys("recvmsg error");
			}
			//return 0 for success,or -1 for failure
			gettimeofday(&tval,NULL);
			Pr->func_proc(recv_buf,recv_num,&msg,&tval);
		}
	}
}
