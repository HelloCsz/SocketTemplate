#include "CszBitTorrent.h"

namespace Csz
{
	bool LocalBitField::CheckBitField(int32_t T_index)

	void LocalBitField::RecvHave(int T_socket,int32_t T_index)
	{
		if (!BitField::CheckPiece(T_index))
		{
			auto need_piece= NeedPiece::GetInstance()->PushNeed(T_index,T_socket);
		}
		return ;
	}

	void LoacalBitField::RecvBitField(int T_socket,const char* T_bit_field,const int T_len)
	{
		if (T_bit_filed.empty())
		{
			Csz::ErrMsg("Local BitField recv empty bit filed");
			return ;
		}
		auto indexs= BitField::LackNeedPiece(T_bit_field,T_len);
		if (indexs.empty())
			return ;
		NeedPiece::GetInstance()->PushNeed(&indexs,T_socket);
		return ;
	}
}
