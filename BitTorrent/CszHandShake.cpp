#include <string.h> //memcpy,strncmp
#include <strings.h> //bzero
#include "CszBitTorrent.h"

namespace Csz
{
	HandShake::HandShake()
	{
#ifdef CszTest
        Csz::LI("constructor Hand Shake");
#endif
		bzero(data,sizeof (data));
		//set pstrlen
		data[0]= 19;
		char BT[]="BitTorrent protocol";
		memcpy(data+1,BT,19);
	}
	bool HandShake::SetParameter(const char* T_reserved,const char* T_info_hash,const char* T_peer_id)
	{
		if (nullptr== T_info_hash || *T_info_hash== 0)
		{
			Csz::ErrMsg("[Hand Shake set parameter]->failed,info hash is empty");
			return false;
		}
		memcpy(data+ 28,T_info_hash,20);
		if (T_reserved!= nullptr && *T_reserved!= 0)
		{
			memcpy(data+ 20,T_reserved,8);
		}
		if (T_peer_id!= nullptr && *T_peer_id!= 0)
		{
			memcpy(data+ 48,T_peer_id,20);
		}
#ifdef CszTest
        Csz::LI("[Hand Shake set parameter]INFO:");
		COutInfo();
#endif
		return true;
	}

	bool HandShake::Varification(const char* T_str) const
	{
		if (T_str== nullptr || *T_str== 0)
        {
#ifdef CszTest
            Csz::LI("[Hand Shake varification]->failed,str is nullpt or point empty");
#endif
			return false;
        }
		if (0== memcmp(T_str,data,20) && 0== memcmp(T_str+28,data+ 28, 20))
		{	
			return true;
			const char* reserved= data+ 20;
			//extension protocol && fast extension
			if ((reserved[5]& 0x10) && (reserved[7]& 0x04))
			{
				return true;
			}
			else
			{
				return false;
			}
        }
		return false;
	}
    
    void HandShake::COutInfo()
    {
        std::string out_info;
        out_info.reserve(64);
        char* p= data;
        out_info.append("[HandShake INFO]:");
        out_info.append("pstrlen="+std::to_string(int(*p)));
        out_info.append(",pstr="+ std::string(p+ 1,19));
        out_info.append(",info_hash="+ Csz::UrlEscape()(std::string(p+ 28,20)));
        out_info.append(",peer_id="+ std::string(p+ 48,20));
		if (!out_info.empty())
			Csz::LI("%s",out_info.c_str());
    }
}
