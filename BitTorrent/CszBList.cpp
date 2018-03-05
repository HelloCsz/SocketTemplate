#include "CszBitTorrent.h"

namespace Csz
{
	BList::BList(){}

	BList::~BList()
	{
#ifdef CszTest
		printf("destructor BList\n");
#endif
		for (auto& val : data)
		{
			delete val;
			val= nullptr;
		}
	}

	void BList::Decode(std::string& T_content)
	{
#ifdef CszTest
		printf("choice l\n");
#endif
		if (T_content.empty() || 'l'!= T_content[0])
		{
			Csz::ErrQuit("BList can't decode,content is empty or string begin character is not 'l'");
			return ;
		}
		//delete l
		T_content.erase(0,1);
		BDataType* base= nullptr;
		while (!T_content.empty() && T_content[0]!= 'e')
		{
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
						Csz::ErrMsg("BList can't new BInt");
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
					catch(...)
					{
						Csz::ErrMsg("BList can't new BLIst");
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
					catch(...)
					{
						Csz::ErrMsg("BList can't new BDict");
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
					catch(...)
					{
						Csz::ErrMsg("BList can't new BStr");
						return ;
					}
				}
			}
			base->Decode(T_content);
			data.push_back(base);
		}
		//stop condition character 'e'
		if (!T_content.empty())
		{
			T_content.erase(0,1);
		}
		return ;
	}

	void BList::ReadData(const std::string& T_name,TorrentFile* T_connect)
	{
		if (data.empty())
			return ;
		auto start= data.begin();
		auto stop= data.end();
		if (T_name== "infofilespath")
		{
			auto path= start;
			++start;
			for (;start< stop; ++start)
			{
				(path->data).append("/");
				(path->data).append(*start);
			}
			path->ReadData(T_name,T_connect);
		}
		else//normal type
		{
			for (const auto& val : data)
			{
				val->ReadData(T_name,T_connect);
			}
		}
		return ;
	}

#ifdef CszTest
	void BList::COutInfo()
	{
		printf("{\n");
		for (const auto& val : data)
		{
			val->COutInfo();
		}
		printf("}\n");
		return ;
	}
#endif
}
