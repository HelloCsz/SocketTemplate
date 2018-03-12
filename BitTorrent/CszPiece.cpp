#include "CszBitTorrent.h"

namespace Csz
{
	void Piece::SetParameter(int32_t T_index,int32_t T_begin,PieceSliceType T_slice)
	{
		if (T_index< 0 || T_begin< 0 || T_slice.empty())
		{
			Csz::ErrMsg("[Piece set parameter]->failed,index or begin less 0");
			return ;
		}
		//not sure host is small endian
		int32_t index= htonl(T_index);
		int32_t begin= htonl(T_begin);
		char* flag= reinterpret_cast<char*>(&index);
		//set network byte use [],data is string
		//set index
		prefix_and_slice[5]= *(flag+ 0);
		prefix_and_slice[6]= *(flag+ 1);
		prefix_and_slice[7]= *(flag+ 2);
		prefix_and_slice[8]= *(flag+ 3);
		//set begin
		flag= reinterpret_cast<char*>(&begin);
		prefix_and_slice[9]= *(flag+ 0);
		prefix_and_slice[10]= *(flag+ 1);
		prefix_and_slice[11]= *(flag+ 2);
		prefix_and_slice[12]= *(flag+ 3);
		//set slice
		prefix_and_slice.append(std::move(T_slice));
		_SetPrefixLength();
		return ;
	}

	inline void Piece::_SetPrefixLength()
	{
		if (prefix_and_slice.size()<= 13)
		{
			Csz::ErrMsg("[Piece set prefix length]->failed,slice too small,can't set prefix length");
			return ;
		}
		//minux prefix length
		//not sure host is small endian
		int32_t temp= htonl(prefix_and_slice.size()- 4);
		//not use static_cast<int32_t*>,data is string
		char* flag= reinterpret_cast<char*>(&temp);
		prefix_and_slice[0]= *(flag+ 0);
		prefix_and_slice[1]= *(flag+ 1);
		prefix_and_slice[2]= *(flag+ 2);
		prefix_and_slice[3]= *(flag+ 3);
		return ;
	}
    
    void Piece::COutInfo()
    {
        std::string out_info;
        out_info.reserve(64);
        char* p= &prefix_and_slice[0];
        out_info.append("[Piece INFO]:len=");
        out_info.append(std::to_string(ntohl(*reinterpret_cast<int32_t*>(p))));
        out_info.append(";id="+ std::to_string(int(*(p+4))));
        out_info.append(";index="+std::to_string(ntohl(*reinterpret_cast<int32_t*>(p+ 5))));
        out_info.append(";begin="+std::to_string(ntohl(*reinterpret_cast<int32_t*>(p+ 9))));
        out_info.append(";msg length="+std::to_string(prefix_and_slice.size()- 13));
		if (!out_info.empty())
			Csz::LI("%s",out_info.c_str());
    }
}
