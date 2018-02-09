#ifndef CszDAMON_H
#define CszDAMON_H
#include <syslog.h> //openlog
#include <sys/resource.h> //getrlimit
#include <cstdlib> //exit
#include <unistd.h> //fork setsid
#include <csignal> //sigaction
#include <fcntl.h> //open
#include "../Error/CszError.h"
namespace Csz
{
	void DaemonInit(const char*,int);
}
#endif
