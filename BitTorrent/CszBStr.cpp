#include "CszBitTorrent.h"
namespace Csz
{
	BStr::BStr()
	{
#ifdef CszTest
		Csz::LI("construct BStr");
#endif
		data.reserve(32);
	}

	BStr::~BStr()
	{
#ifdef CszTest
		Csz::LI("destructor BStr");
#endif
	}

	void BStr::Decode(std::string& T_content)
	{
#ifdef CszTest
		Csz::LI("choice string");
#endif
		if (T_content.empty())
		{
			Csz::ErrMsg("BStr can't decode,content is empty");
			return ;
		}
		auto mid= T_content.find_first_of(':');
		if (mid== T_content.npos)
		{
			Csz::ErrQuit("BStr can't decode,not found ':'");
			return ;
		}
		size_t num;
		try
		{
			num= std::stoul(T_content.substr(0,mid));
			if ((num+mid+1)> T_content.size())
				Csz::ErrQuit("%lu greater than string size %lu",num+mid+1,T_content.size());
		}
		catch (std::out_of_range& e)
		{
			Csz::ErrQuit("BStr can't decode,string %s out of range,%s",T_content.substr(0,mid).c_str(),e.what());
			return ;
		}
		catch (std::invalid_argument& e)
		{
			Csz::ErrQuit("BStr can't decode,string %s invalid,%s",T_content.substr(0,mid).c_str(),e.what());
			return ;
		}
		catch (...)
		{		
			Csz::ErrQuit("BStr can't decode,string %s unknow error,%s",T_content.substr(0,mid).c_str());
			return ;
		}
		data= T_content.substr(mid+ 1,num);
		T_content.erase(0,num+mid+ 1);
		return ;
	}

	void BStr::ReadData(const std::string& T_name,TorrentFile* T_torrent)
	{
		auto result= (*T_torrent).name_data.find(T_name);
		if ((*T_torrent).name_data.end()== result)
			return ;
		(result->second)((void*)&data);
	}

	void BStr::COutInfo()
	{
		if (data.size()< 128)
			Csz::LI("Str:%s\n",data.c_str());
		else
		{
			Csz::LI("Str data greater than 256,size= %lu\n",data.size());
			//write(2,data.c_str(),data.size());
		}
	}
#endif
}
