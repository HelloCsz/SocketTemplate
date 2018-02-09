#include "CszDebugIp.h"
namespace Csz
{
	void EchoIcmp4(const struct icmp* T_icmp)
	{
		std::cout<<"info icmp:\n";
		std::cout<<"type:"<<(int)T_icmp->icmp_type<<",code:"<<(int)T_icmp->icmp_code<<",check sum:"<<T_icmp->icmp_cksum<<"\n";
		std::cout<<"icmp ip:"<<T_icmp->icmp_id<<",icmp seq:"<<T_icmp->icmp_seq<<"\n";
	}
}
