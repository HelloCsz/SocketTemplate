#include "CszPing.h"

namespace Csz
{
	//Mike Muuss public domain
	uint16_t InCksum(uint16_t* T_data,int T_data_len)
	{
		int nleft= T_data_len;
		uint32_t sum= 0;
		uint16_t* w= T_data;
		uint16_t answer= 0;
		//our algorithm is simple,using a 32 bit accumulator(sum),we add
		//sequential 16 bit words to is,and at the end,fold back all the
		//carry bits form the top 16 bits into the lower 16 bit
		while (nleft> 1)
		{
			sum+= *w++;
			nleft-= 2;
		}
		//mop up an odd byte,if sucessary
		if (1== nleft)
		{
			*(unsigned char*)(&answer)= *(unsigned char*)w;
			sum+= answer;
		}
		//add back carry outs from top 16 bits to low 16 bit
		sum= (sum>> 16)+ (sum& 0xffff);
		//add carry
		sum+= (sum>> 16);
		//truncate to bits
		answer= ~sum;
		return answer;
	}
}
