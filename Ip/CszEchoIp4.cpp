#include "CszDebugIp.h"

namespace Csz
{
	void EchoIp4(const struct ip* T_ip)
	{
		std::cout<<"info ip:\n";
		std::cout<<"version:"<<T_ip->ip_v<<",head len:"<<T_ip->ip_hl<<",tos:"<<(int)T_ip->ip_tos<<",total:"<<T_ip->ip_len<<"\n";
		std::cout<<"id:"<<T_ip->ip_id<<",off:"<<T_ip->ip_off<<"\n";
		std::cout<<"ttl:"<<(int)T_ip->ip_ttl<<",protocol:"<<(int)T_ip->ip_p<<",check sum:"<<T_ip->ip_sum<<"\n";
		std::cout<<"src:"<<inet_ntoa(T_ip->ip_src)<<"des:"<<inet_ntoa(T_ip->ip_dst)<<"\n";
	}
}
