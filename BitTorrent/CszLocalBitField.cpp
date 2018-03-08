#include <bitset>
#include "CszBitTorrent.h"

namespace Csz
{
	bool LocalBitField::CheckBitField(int32_t T_index)
    {
        //may be bit field size< 5
        if (bit_field.CheckPiece(T_index) && T_index>= 0)
            return true;
        return false;
    }

	void LocalBitField::RecvHave(int T_socket,int32_t T_index)
	{
		if (!bit_field.CheckPiece(T_index))
		{
			NeedPiece::GetInstance()->PushNeed(T_index,T_socket);
		}
		return ;
	}

	void LocalBitField::RecvBitField(int T_socket,const char* T_bit_field,const int T_len)
	{
		if (nullptr== T_bit_field)
		{
			Csz::ErrMsg("Local BitField recv empty bit filed");
			return ;
		}
		auto indexs= bit_field.LackNeedPiece(T_bit_field,T_len);
		if (indexs.empty())
			return ;
		NeedPiece::GetInstance()->PushNeed(&indexs,T_socket);
		return ;
	}
    
    void LocalBitField::FillBitField(int32_t T_index)
    {
#ifdef CszTest
        COutInfo();
#endif
        bit_field.FillBitField(T_index);
        PeerManager::GetInstance()->SendHave(T_index);
        return ;
    }

	void LocalBitField::COutInfo() const
	{
		std::bitset<8> bit_set(end_bit);
		Csz::LI("Local Bit Field info:end bit %s",bit_set.to_string().c_str());
		bit_field.COutInfo();
		return ;
	}
}
