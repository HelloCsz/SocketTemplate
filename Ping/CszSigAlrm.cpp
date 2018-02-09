#include "CszPing.h"

extern Csz::Proto* Pr; //main.cpp

namespace Csz
{
	void SigAlrm(int T_sig_no)
	{
		Pr->func_send();
		alarm(1);
		return ;
	}
}
