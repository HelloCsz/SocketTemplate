#include <syslog.h> //openlog
#include <sys/resource.h> //getrlimit
#include <cstdlib> //exit
#include <unistd.h> //fork setsid
#include <csignal> //sigaction
#include <fcntl.h> //open
#include "CszDaemon.h"

//extern int deamon_proc; //defined in CszError.cpp
//修改 /ect/rsyslog.d/*.conf 文件后需要 service rsyslog restart
namespace Csz
{
	extern int daemon_proc;
	//守护进程不会收到来自内核SIGHUP,SIGINT,SIGWINCH
	void DaemonInit(const char* T_name,int T_facility)
	{
		pid_t pid;
		if ((pid= fork())< 0)
		{
			ErrSys("%s:can't fork",T_name);
		}
		else if (pid!= 0)
		{
			//关闭父进程
			exit(0);
		}
		//继承父进程的进程组,且不是进程组的头进程
		//child one continues
		//become session leader
		if (setsid()< 0)
		{
			ErrSys("%s:can't setsid",T_name);
		}
		//新会话头进程且新进程组头进程
		//忽略sighup信号再调用fork,父进程exit后所有子进程将收到sighup
		struct sigaction sa;
		sa.sa_handler= SIG_IGN;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags= 0;
		if (sigaction(SIGHUP,&sa,NULL)< 0)
		{
			ErrSys("%s:can't ignore SIGHUP",T_name);
		}
		//一个没有控制终端的会话头进程打开一个终端设备,该终端自动成为这个会话头进程的控制终端
		//确保守护进程将来即使打开一个终端设备也不会自动获得控制终端
		if ((pid= fork())< 0)
		{
			ErrSys("%s: second can't fork",T_name);
		}
		else if (pid!= 0)
			exit(0);
		//确保子进程不是会话头进程
		//守护进程可能是某个任意文件系统启动,该文件系统不能拆卸
		if (chdir("/home/hello-wxx/")< 0)
		{
			ErrSys("%s: can't change directory to /home/hello-wxx/",T_name);
		}
		daemon_proc= 1;
		struct rlimit rl;
		if (getrlimit(RLIMIT_NOFILE,&rl)< 0)
		{
			ErrMsg("%s:can't get file limit",T_name);
			rl.rlim_max= 1024;
		}
		//关闭继承来的所有fd
		for (int i= 0; i< rl.rlim_max; ++i)
			close(i);
		//重定向标准输入流,服务器打开与某天客户直接socket,描述符将从最低数字开始(0,1,2)
		//那么若某个底层系统调用调用了i/o函数,就把数据发往客户
		open("/dev/null",O_RDONLY);
		open("/dev/null",O_RDWR);
		open("dev/null",O_RDWR);
		openlog(T_name,LOG_PID,T_facility);
		return ;
	}
}
