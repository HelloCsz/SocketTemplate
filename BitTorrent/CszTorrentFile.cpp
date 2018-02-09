#include "CszBitTorrent.h"

namespace Csz
{
	TorrentFile::TorrentFile():creation_date(0)
	{
		name_data["announce"]=std::function<void(void*)>([this](void*T_data)
				{
					announce_list.push_back(std::move(*((std::string*)T_data)));
				});
		name_data["announce-list"]= std::function<void(void*)>([this](void* T_data)
				{
					announce_list.push_back(std::move(*((std::string*)T_data)));
				});
		name_data["comment"]=std::function<void(void*)>([this](void*T_data)
				{
					comment= std::move(*((std::string*)T_data));
				});
		name_data["create by"]= std::function<void(void*)>([this](void*T_data)
				{
					create_by= std::move(*((std::string*)T_data));
				});

		name_data["creation data"]=std::function<void(void*)>([this](void*T_data)
				{
					creation_date= *(std::uint64_t*)T_data;
				});

		name_data["infopiece length"]=std::function<void(void*)>([this](void*T_data)
				{
					infos.piece_length= *(std::uint64_t*)T_data;
				});
		name_data["infopieces"]= std::function<void(void*)>([this](void*T_data)
				{
					infos.pieces= std::move(*((std::string*)T_data));
				});

		name_data["infofileslength"]=std::function<void(void*)>([this](void*T_data)
				{
					if (infos.files.empty())
					{
						
						infos.files.emplace_back("",*((std::uint64_t*)T_data));
					}
					else if (infos.files[infos.files.size()-1].length== 0)
					{
						infos.files[infos.files.size()-1].length=*(std::uint64_t*)T_data;
					}
					else
					{
						infos.files.emplace_back("",*((std::uint64_t*)T_data));
					}
				});
		name_data["infofilespath"]= std::function<void(void*)>([this](void*T_data)
				{
					//多文件模式
					infos.single= false;
					if (infos.files.empty())
					{
						infos.files.emplace_back(*(std::string*)T_data,0);
					}
					else if (infos.files[infos.files.size()- 1].file_path.size()== 0)
					{
						infos.files[infos.files.size()- 1].file_path= std::move(*((std::string*)T_data));
					}
					else
					{
						infos.files.emplace_back(*(std::string*)T_data,0);
					}
				});
		
		name_data["infoname"]= std::function<void(void*)>([this](void* T_data)
				{
					infos.name= std::move(*(std::string*)T_data);
				});
		name_data["infolength"]= std::function<void(void*)>([this](void* T_data)
				{
					//单文件,不在name中判断是多文件也有name tap
					infos.single= true;
					infos.length= *(std::uint64_t*)T_data;
				});
	}
	TorrentFile::~TorrentFile()
	{
#ifdef CszTest
		printf("destructor TorrentFile\n");
#endif
	}
	void TorrentFile::GetTrackInfo(Tracker* T_tracker)
	{
		if (announce_list.empty())
		{
			Csz::ErrQuit("TorrentFile can't return info,announce is empty");
			return ;
		}
		for (const auto& val : announce_list)
		{
			struct TrackerInfo data;
			auto host_flag= val.find("//");
			auto uri_flag= val.find_first_of('/',host_flag+ 2);
			auto serv_flag= val.find(':',host_flag);
			if (val.npos== host_flag || val.npos== uri_flag)
			{
				Csz::ErrQuit("TorrentFile can't catch host or announce,string:%s",val.c_str());
				return ;
			}
			// //*.*.*:/
			if (val.npos== serv_flag)
				data.host.assign(val.substr(host_flag+ 2,uri_flag- host_flag- 2));
			else
				data.host.assign(val.substr(host_flag+ 2,serv_flag- host_flag- 2));
			if (val.npos== serv_flag)
			{
				data.serv.assign("http");
			}
			else
			{
				if (val.npos== uri_flag)
				{
					Csz::ErrQuit("TorrentFile can't found serv %s",val.c_str());
					return ;
				}
				//:54321
				data.serv.assign(val.substr(serv_flag+ 1,uri_flag- serv_flag- 1));
			}
			data.uri.assign(val.substr(uri_flag));
			//tcp set -1.udp set -2
			if ((host_flag= val.find("udp"))== val.npos)
				data.socket_fd= -1;
			else
				data.socket_fd= -2;
			T_tracker->SetTrackInfo(std::move(data));
		}
	}
	std::string TorrentFile::GetIndexHash(int T_index)
	{
		if ((T_index+ 1)* 20 > (int)infos.pieces.size() || T_index< 0)
		{
			Csz::ErrQuit("TorrentFile can't get hash,index %d",T_index);
			return "";
		}
		return infos.pieces.substr(T_index,20);
	}
	std::uint64_t TorrentFile::GetTotal()const
	{
		uint64_t ret= 0;
		if (infos.single)
		{
			ret= infos.length;
		}
		else
		{
			for (const auto& val : infos.files)
			{
				ret+= val.length;
			}
		}
		return ret;
	}
#ifdef CszTest
	void TorrentFile::COutInfo()
	{
		/*
		if (!announce.empty())
		{
			printf("announce:%s\n",announce.c_str());
		}
		*/
		if (!announce_list.empty())
		{
			printf("announce or announce_list:\n");
			for (const auto& val : announce_list)
			{
				printf("%s\n",val.c_str());
			}
		}
		if (!comment.empty())
		{
			printf("comment:%s\n",comment.c_str());
		}
		if (creation_date)
		{
			printf("creation date:%lu\n",creation_date);
		}
		if (!create_by.empty())
		{
			printf("create by:%s\n",create_by.c_str());
		}
		if (infos.piece_length)
		{
			printf("piece length:%lu\n",infos.piece_length);
		}
		if (!infos.pieces.empty())
		{
			printf("pieces size:%lu\n",infos.pieces.size());
		}
		if (infos.single)
		{
			printf("single mode:\n");
		}
		else
		{
			printf("multi mode:\n");
		}
		if (!infos.name.empty())
		{
			printf("name:%s\n",infos.name.c_str());
		}
		if (infos.length)
		{
			printf("length:%lu\n",infos.length);
		}
		if (!infos.files.empty())
		{
			for (const auto& val : infos.files)
			{
				if (!val.file_path.empty())
				{
					printf("file path:%s\n",val.file_path.c_str());
				}
				if (val.length)
				{
					printf("length:%lu\n",val.length);
				}
			}
		}
	}
#endif
}
