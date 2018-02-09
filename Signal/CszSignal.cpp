#include "CszSignal.h"

namespace Csz
{
	SigHandler Signal(int T_signum,SigHandler T_handler)
	{
		struct sigaction act,old_act;
		act.sa_handler= T_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags= 0;
		//signal alarm一般是socket发送超时,这时候如果继续重启
		//则可能永远阻塞
		if (SIGALRM== T_signum)
		{
#ifdef  SA_INTERRUPT
			act.sa_flags|= SA_INTERRUPT;
#endif
		}
		else
		{
#ifdef SA_RESTART
			act.sa_flags|= SA_RESTART;
#endif
		}
		if (sigaction(T_signum,&act,&old_act)< 0)
			return SIG_ERR;
		return old_act.sa_handler;
	}
}
