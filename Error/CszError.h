#ifndef CszError_H
#define CszError_H
#include <cerrno> //errno
#include <cstdarg> //va_start,va_arg,va_end,va_list,vsnprintf,vsprintf
#include <syslog.h> //syslog,openlog,closelog
#include <cstdlib> //abort
#include <cstring> //strlen
#include <cstdio> //snprintf,fflush,fputs

namespace Csz
{
	void ErrDump(const char*,...);
	void ErrSys(const char*,...);
	void ErrRet(const char*,...);
	void ErrMsg(const char*,...);
	void ErrQuit(const char*,...);
    void LI(const char*,...);
}

#endif
