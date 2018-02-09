#include "CszPing.h"

namespace Csz
{
	void TvSub(struct timeval* T_last,struct timeval* T_first)
	{
		if ((T_last->tv_usec-= T_first->tv_usec)< 0)
		{
			--T_last->tv_sec;
			T_last->tv_usec+= 1000000;
		}
		T_last->tv_sec-= T_first->tv_sec;
	}
}
