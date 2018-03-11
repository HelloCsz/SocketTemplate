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
			auto id= PeerManager::GetInstance()->GetSocketId(T_socket);
			if (id< 0)
			{
#ifdef CszTest
				Csz::LI("Local Bit Field recv have failed,not found socket id");
#endif
				return ;
			}
			PeerManager::GetInstance()->AmInterested(T_socket);
			NeedPiece::GetInstance()->PushNeed(T_index,T_socket,id);
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
		auto id= PeerManager::GetInstance()->GetSocketId(T_socket);
		if (id< 0)
		{
#ifdef CszTest
			Csz::LI("Local Bit Field recv bit field failed,not found socket id");
#endif
			return ;
		}
		PeerManager::GetInstance()->AmInterested(T_socket);
		NeedPiece::GetInstance()->PushNeed(&indexs,T_socket,id);
		return ;
	}
    
    void LocalBitField::FillBitField(int32_t T_index)
    {
        bit_field.FillBitField(T_index);
        PeerManager::GetInstance()->SendHave(T_index);
#ifdef CszTest
        COutInfo();
#endif
        return ;
    }

	void LocalBitField::COutInfo() const
	{
        Csz::LI("Local Bit Field INFO:");
		bit_field.COutInfo();
		return ;
	}
}
