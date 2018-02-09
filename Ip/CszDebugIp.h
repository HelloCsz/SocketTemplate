#ifndef CszDEUBGIP_H
#define CszDEBUGIP_H
#include <iostream>
#include <arpa//inet.h> //inet_ntoa
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
namespace Csz
{
	void EchoIp4(const struct ip*);
	void EchoIcmp4(const struct icmp*);
}
#endif
