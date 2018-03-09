#include "CszBitTorrent.h"

namespace Csz
{
	BDict::BDict()
	{
#ifdef CszTest
//		Csz::LI("constrctor BDict");
#endif
	}

	BDict::~BDict()
	{
#ifdef CszTest
//		Csz::LI("destructor BDict");
#endif
		for (auto& val : data)
		{
			delete val.second;
			val.second= nullptr;
		}
	}

	void BDict::Decode(std::string& T_content)
	{
#ifdef CszTest
//			Csz::LI("choice d");
#endif
		if (T_content.empty() || T_content[0]!= 'd')
		{
			Csz::ErrQuit("BDict can't decode,content is empty or content begin character not is 'd'");
			return ;
		}
		//delete d
		T_content.erase(0,1);
		BDataType* base= nullptr;
		while (!T_content.empty() && T_content[0]!= 'e')
		{
			BStr key;
			key.Decode(T_content);
			switch (T_content[0])
			{
				case 'i':
					{
						try
						{	
							base= new BInt();
							break;
						}
						catch (...) //std::bad_alloc,std::bad_array_new_length
						{
							Csz::ErrMsg("BDict can't new BInt");
							return ;
						}
					}
				case 'l':
					{
						try
						{
							base= new BList();
							break;
						}
						catch (...) //std::bad_alloc,std::bad_array_new_length
						{
							Csz::ErrMsg("BDict can't new BList");
							return ;
						}
					}
				case 'd':
					{
						try
						{
							base= new BDict();
							break;
						}
						catch (...) //std::bad_alloc,std::bad_array_new_length
						{
							Csz::ErrMsg("BDict can't new BDict");
							return ;
						}
					}
				default:
					{
						try
						{	
							base= new BStr();
							break;
						}
						catch (...) //std::bad_alloc,std::bad_array_new_length
						{
							Csz::ErrMsg("BDict can't new BStr");
							return ;
						}
					}
			}
			base->Decode(T_content);
			auto check= data.emplace(std::move(key.data),base);
			if (!check.second)
			{
				Csz::ErrMsg("dumplicate message %s",(check.first)->first.c_str());
			}
			//data.emplace(std::move(key),base);
		}
		if (!T_content.empty())
		{
			T_content.erase(0,1);
		}
		return ;
	}

	/*
	BDataType* BDict::Search(std::string& T_name) const
	{
		auto result= data.find(T_name);
		if (data.end()== result)
			return nullptr;
		return result->second;
	}
	*/

	void BDict::ReadData(const std::string& T_name,TorrentFile* T_torrent)
	{
		if (data.empty())
			return ;
		for (const auto& val : data)
		{
			std::string temp(T_name);
			(val.second)->ReadData(temp.append(val.first),T_torrent);
		}
	}

	void BDict::COutInfo()
	{
		for (const auto& val : data)
		{
			Csz::LI("Diec:key:%s->",val.first.c_str());
			(val.second)->COutInfo();
			//Csz::ErrMsg("\n");
		}
	}
}
