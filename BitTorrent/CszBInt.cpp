#include "CszBitTorrent.h"

namespace Csz
{
	BInt::BInt() : data(0)
	{
#ifdef CszTest
		Csz::LI("constructor BInt");
#endif
	}

	BInt::~BInt()
	{
#ifdef CszTest
		Csz::LI("destructor BInt");
#endif
	}

	void BInt::Decode(std::string& T_content)
	{
#ifdef CszTest
		Csz::LI("choice int");
#endif
		if (T_content.empty() || T_content[0]!= 'i')
		{
			Csz::ErrQuit("BInt can't decode,content is empty or content start character not 'i'");
			return ;
		}
		auto stop= T_content.find_first_of('e');
		if (stop== T_content.npos)
		{
			Csz::ErrQuit("BInt can't decode,not found end-character");
			return ;
		}
		try
		{	
			//b编码int类型首字符'i'没有删除
			//substr第一个参数position,第二个参数是数量,stop是index
			data= std::stoul(T_content.substr(1,stop- 1));
		}
		catch(std::out_of_range& e)
		{
			Csz::ErrQuit("BInt can't decode,string %s out of range ,%s",T_content.substr(1,stop).c_str(),e.what());
			return ;
		}
		catch (std::invalid_argument& e)
		{
			Csz::ErrQuit("BInt can't decode,string %s invalid,%s",T_content.substr(1,stop).c_str(),e.what());
			return ;
		}
		catch(...)
		{
			Csz::ErrQuit("BInt can't decode,string %s,unknow error",T_content.substr(1,stop).c_str());
			return ;
		}
		//参数1是position,参数2是数量
		//stop为字符'e'的index
		T_content.erase(0,stop+ 1);
		return ;
	}

	void BInt::ReadData(const std::string& T_name,TorrentFile* T_torrent)
	{
		auto result= (*T_torrent).name_data.find(T_name);
		if ((*T_torrent).name_data.end()== result)
			return ;
		(result->second)((void*)&data);
		return ;
	}

	void BInt::COutInfo()
	{
		Csz::LI("Int:%lu",data);
		return ;
	}
}
