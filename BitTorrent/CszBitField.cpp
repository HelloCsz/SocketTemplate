#include "CszBitTorrent.h"

namespace Csz
{
	void BitField::SetParameter(std::string T_bit_field)
	{
		_Clear();
		_SetParameter(std::move(T_bit_field));
		return ;
	}

	inline void BitField::_SetParameter(std::string T_bit_field)
	{
		if (T_bit_field.empty())
		{
			Csz::ErrMsg("bit field is empty");
			return ;
		}
		prefix_and_bit_field.append(std::move(T_bit_field));
		_SetPrefixLength();
		return ;
	}

	inline void BitField::_SetPrefixLength()
	{
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("bit field too small,can't set prefix length");
			return ;
		}
		//minus prefix length
		//not sure host is small endian
		int32_t temp= htonl(prefix_and_bit_field.size()- 4);
		//host change network byte
		//not use static_cast<int32_t*>,prefix_and_bit_field is string
		char* flag= reinterpret_cast<char*>(&temp);
		prefix_and_bit_field[0]= *(flag+ 0);
		prefix_and_bit_field[1]= *(flag+ 1);
		prefix_and_bit_field[2]= *(flag+ 2);
		prefix_and_bit_field[3]= *(flag+ 3);
		return ;
	}

	inline void BitField::_Clear()
	{
		prefix_and_bit_field.erase(5);
		return ;
	}

	void BitField::FillBitField(int32_t T_index)
	{
		if (T_index< 0)
		{
			Csz::ErrMsg("index < 0,can't fill bit field");
			return ;
		}
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("bit field too small,can't fill bit field");
			return ;
		}
		//every bit express one field
		auto index= T_index/ 8+ 5;
		auto& val= prefix_and_bit_field[index];
		switch (T_index% 8)
		{
			case 0:
				val|= 0x01;
				break;
			case 1:
				val|= 0x02;
				break;
			case 2:
				val|= 0x04;
				break;
			case 3:
				val|= 0x08;
				break;
			case 4:
				val|= 0x10;
				break;
			case 5:
				val|= 0x20;
				break;
			case 6:
				val|= 0x40;
				break;
			case 7:
				val|= 0x80;
				break;
		}
		return ;
	}

	bool BitField::CheckPiece(int32_t T_index)
	{
		if (T_index< 0)
		{
			Csz::ErrMsg("Check Piece failed,index < 0");
			return true;
		}
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("bit field too small,not found index");
			return true;
		}
		//every bit express one field
		auto index= T_index/ 8+ 5;
		auto& val= prefix_and_bit_field[index];
		bool ret= false;
		switch (T_index% 8)
		{
			case 0:
				ret= val& 0x01;
				break;
			case 1:
				ret= val& 0x02;
				break;
			case 2:
				ret= val& 0x04;
				break;
			case 3:
				ret= val& 0x08;
				break;
			case 4:
				ret= val& 0x10;
				break;
			case 5:
				ret= val& 0x20;
				break;
			case 6:
				ret= val& 0x40;
				break;
			case 7:
				ret= val& 0x80;
				break;
		}
		return ret;
	}

	std::vector<int32_t> BitField::LackNeedPiece(const char* T_bit_field,const int T_len)
	{
		std::vector<int32_t> ret;
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("bit field too small,found lack need piece failed");
			return ret;
		}
		if (nullptr== T_bit_field || T_len+ 5!= prefix_and_bit_field.size() )
		{
			Csz::ErrMsg("Bit Field not found bit field,bit field is nullptr or len != bit field len");
			return ret;
		}
		int cur_len= 0;
		int32_t index= 0;
		while (cur_len< T_len)
		{
			uint8_t bit= 0x01;
			for (int i= 0; i< 8; ++i,++index)
			{
				//1.have piece
				if(T_bit_field[cur_len]& bit)
				{
					//2.check local bit
					if (prefix_and_bit_field[cur_len] & bit)
					{
						//2.1 loca have piece
						bit<<= 1;
						continue ;
					}
					//2.2local lack piece
					ret.emplace_back(index);
				}
				bit<<= 1;
			}
			++cur_len;
		}
		return std::move(ret);
	}

	bool BitField::GameOver(const char T_end_bit)const
	{
		if (prefix_and_bit_field.size()<= 5)
		{
			return false;
		}
		for (auto start= prefix_and_bit_field.cbegin()+ 5,stop= prefix_and_bit_field.cend(); start< stop; ++start)
		{
			const char temp= 0xff;
			if (*start!= temp)
			{
				if (start== (stop- 1) && T_end_bit!= 0x00 && *start== T_end_bit)
					return true;
				return false;
			}
		}
		return true;
	}
}
