#include "CszBitTorrent.h"

namespace Csz
{
	//d***
	int GetDictCount::operator()(const char* T_str,int T_len)const
	{
		if (nullptr== T_str || T_len<= 0)
			return -1;
		if (*T_str!= 'd')
			return -1;
		const char* save_str= T_str;;
		++T_str;
		int count= 1;
		while (count> 0 && T_len--)
		{
			switch (*T_str)
			{
				case 'd':
				case 'l':
					++count;
					break;
				case 'e':
					--count;
					break;
				case 'i':
					{
						++T_str;
						while (isdigit(*T_str))
						{
							++T_str;
						}
						//stop in 'e'
					}
					break;
				default:
					{
						int num= 0;
						while (isdigit(*T_str))
						{
							num= num* 10+ (*T_str- '0');
							++T_str;
						}
						if (0== num)
						{
							return -1;
						}
						//8:announce
						T_str+= num;
					}
					break;
			}
			++T_str;
		}
		return (T_str- save_str);
	}
}
