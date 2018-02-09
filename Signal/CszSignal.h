#ifndef CszSIGNAL_H
#define CszSIGNAL_H

#include <csignal> //sigaction,sigempty


namespace Csz
{
	using SigHandler= void (*)(int);
	SigHandler Signal(int,SigHandler);
	SigHandler SignalAlarm(int,SigHandler);
}

#endif
