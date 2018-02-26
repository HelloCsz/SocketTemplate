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
}
