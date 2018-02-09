#include "CszWeb.h"

namespace Csz
{
	std::string UrlEscape::operator()(std::string T_str)
	{
		std::string temp;
		temp.reserve(64);
		int size= T_str.size();
		for (int i= 0; i< size; ++i)
		{
			if (isalnum(T_str[i])|| '.'== T_str[i] || '-'== T_str[i] || '_'== T_str[i] || '~'== T_str[i])
				temp.append(1,T_str[i]);
			else
			{
				char ch[4]={0};
				snprintf(ch,4,"%%%.2x",T_str[i]&0xff);
				temp.append(ch,3);
			}
		}
		return temp;
	}
}
