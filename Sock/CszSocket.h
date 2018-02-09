#ifndef CszSOCKET_H
#define CszSOCKET_H

#if __cplusplus >= 201103L
#include <memory> //shared_ptr 
#endif

#include <sys/types.h>
#include <sys/socket.h>//socket,connect
#include <netdb.h> //getaddrinfo,freeaddrinfo,gai_strerror
#include <strings.h> //bzero
#include <arpa/inet.h> //inet_ntop,inet_pton,ntohs,ntohl,htonl,htons
#include <cstdio> //snprintf,printf
#include <cstring> //strncat
#include <unistd.h> //colse,
#include <fcntl.h> //fcntl
#include "../Error/CszError.h"
#include "../Signal/CszSignal.h"

namespace Csz
{
	//host,serv,family,socktype,error return NULL
	struct addrinfo* SocketHostServ(const char*,const char*,int,int);
	struct addrinfo* SocketHostServNull(const char*,const char*,int,int);
#if __cplusplus >= 201103L
	//error,info CszSocketHostServShared.cpp
	std::shared_ptr<struct addrinfo> SocketHostServShared(const char*,const char*,int,int);
#endif	
	char* SocketNtoP(const struct sockaddr*,socklen_t);
	char* SocketNtoPHost(const struct sockaddr*,socklen_t);
	int CreateSocket(int,int,int);
	int TcpConnect(const char*,const char*);
	int TcpConnectTime(const char*,const char*,int);
	int Fcntl(int,int,int);
}
#endif
