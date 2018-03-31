#include <bitset>
#include "CszBitTorrent.h"

namespace Csz
{
	bool LocalBitField::CheckBitField(int32_t T_index)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //may be bit field size< 5
        if (bit_field.CheckPiece(T_index) && T_index>= 0)
            return true;
        return false;
    }

	void LocalBitField::RecvHave(int T_socket,int32_t T_index)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (!bit_field.CheckPiece(T_index))
		{
			auto id= PeerManager::GetInstance()->GetSocketId(T_socket);
			if (id< 0)
			{
#ifdef CszTest
				Csz::LI("[Local Bit Field recv have]->failed,not found socket id");
#endif
				return ;
			}
            //TODO recursion mutex
#ifdef CszTest
            Csz::LI("[Local Bit Field->../BitTorrent/CszNeedPiece.cpp->PushNeed->12]");
#endif
			PeerManager::GetInstance()->AmInterested(T_socket);
			NeedPiece::GetInstance()->PushNeed(T_index,T_socket,id);
		}
		return ;
	}

	void LocalBitField::RecvBitField(int T_socket,const char* T_bit_field,const int T_len)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (nullptr== T_bit_field)
		{
			Csz::ErrMsg("[Local BitField recv bit field]->failed,bit filed is nullptr");
			return ;
		}
		auto indexs= bit_field.LackNeedPiece(T_bit_field,T_len);
		if (indexs.empty())
			return ;
		auto id= PeerManager::GetInstance()->GetSocketId(T_socket);
		if (id< 0)
		{
#ifdef CszTest
			Csz::LI("[Local Bit Field recv bit field]->failed,not found socket id");
#endif
			return ;
		}
		PeerManager::GetInstance()->AmInterested(T_socket);
		NeedPiece::GetInstance()->PushNeed(&indexs,T_socket,id);
		return ;
	}
    
    void LocalBitField::FillBitField(int32_t T_index)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        bit_field.FillBitField(T_index);
        PeerManager::GetInstance()->SendHave(T_index);
#ifdef CszTest
        Csz::LI("[Local Bit Field fill bit field]INFO:");
        COutInfo();
#endif
        return ;
    }

	void LocalBitField::Clear()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		bit_field.Clear();
		return ;
	}

	void LocalBitField::COutInfo()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        Csz::LI("[Local Bit Field INFO]:");
		bit_field.ProgressBar();
		return ;
	}
}
