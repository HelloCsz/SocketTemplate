#include "CszError.h"

//set nonzero by DaemonInit()
//int daemon_proc= 0;

namespace Csz
{
	#define STRBUFMAX 4096
	//set nonzero by DaemonInit()
	int daemon_proc= 0;
	//static限制ErrDoit函数的作用范围
	static void ErrDoit(int,int,const char*,va_list); 
	//fatal error related to system call
	void ErrDump(const char* T_str,...)
	{
		va_list ap;
		va_start(ap,T_str);
		ErrDoit(1,LOG_ERR,T_str,ap);
		va_end(ap);
		//dump core and terminate
		abort(); 
		//shouldn't get here'
		exit(1);
		return ;
	}

	//fatal error related to system call
	void ErrSys(const char* T_str,...)
	{
		va_list ap;
		va_start(ap,T_str);
		ErrDoit(1,LOG_ERR,T_str,ap);
		va_end(ap);
		exit(1);
		return ;
	}

	//nonfatal error related to system call
	void ErrRet(const char* T_str,...)
	{
		va_list ap;
		va_start(ap,T_str);
		ErrDoit(1,LOG_NOTICE,T_str,ap);
		va_end(ap);
		return ;
	}

	//nonfatal error unrelated to system call
	void ErrMsg(const char* T_str,...)
	{
		va_list ap;
		va_start(ap,T_str);
		ErrDoit(0,LOG_NOTICE,T_str,ap);
		va_end(ap);
		return ;
	}

	//fatal error unrelated to system call
	void ErrQuit(const char* T_str,...)
	{
		va_list ap;
		va_start(ap,T_str);
		ErrDoit(0,LOG_ERR,T_str,ap);
		va_end(ap);
		exit(1);
		return ;
	}

	void LI(const char* T_str,...)
	{
		va_list ap;
		va_start(ap,T_str);
		ErrDoit(0,LOG_INFO,T_str,ap);
		va_end(ap);
		return ;
	}

	void ErrDoit(int T_error_flag,int T_level,const char* T_str,va_list T_ap)
	{
		int errno_save= errno;
		char str_buf[STRBUFMAX]={0};
#ifdef HAVE_VSNPRINTF
		//safe
		vsnprintf(str_buf,sizeof(str_buf),T_str,T_ap);
#else
		//not safe
		vsprintf(str_buf,T_str,T_ap);
#endif
		int cur_len= strlen(str_buf);
		if (T_error_flag)
			snprintf(str_buf+ cur_len,STRBUFMAX- cur_len,":%s",strerror(errno_save));
		//覆盖'\0'
		strcat(str_buf,"\n");
		if (daemon_proc)
			syslog(T_level | LOG_LOCAL3,str_buf,'\0'); //消息发送进程类型facility,默认LOG_USER,'\0'取决与实现(这个版本提示格式错误)
		else
		{
			//先刷新标准输出流
			fflush(stdout);
			fputs(str_buf,stderr);
			fflush(stderr);
		}
		return ;
	}
}
