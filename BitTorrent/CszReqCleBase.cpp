#include "CszBitTorrent.h"

namespace Csz
{
	void ReqCleBase::SetParameter(int32_t T_index,int32_t T_begin,int32_t T_length)
	{
		int32_t index= htonl(T_index);
		int32_t begin= htonl(T_begin);
		int32_t length= htonl(T_length);
		*reinterpret_cast<int32_t*>(data+ 5)= index;
		*reinterpret_cast<int32_t*>(data+ 9)= begin;
		*reinterpret_cast<int32_t*>(data+ 13)= length;
		return ;
	}
    
    void ReqCleBase::COutInfo()
    {
        std::string out_info;
        out_info.reserve(64);
        char* p= data;
        out_info.append("[ReqCleBase INFO]:len=");
        out_info.append(std::to_string(ntohl(*reinterpret_cast<int32_t*>(p))));
        out_info.append(";id="+ std::to_string(int(*(p+4))));
        out_info.append(";index="+std::to_string(ntohl(*reinterpret_cast<int32_t*>(p+ 5))));
        out_info.append(";begin="+std::to_string(ntohl(*reinterpret_cast<int32_t*>(p+ 9))));
        out_info.append(";length="+std::to_string(ntohl(*reinterpret_cast<int32_t*>(p+ 13))));
		if (!out_info.empty())
			Csz::LI("%s",out_info.c_str());
    }
}
