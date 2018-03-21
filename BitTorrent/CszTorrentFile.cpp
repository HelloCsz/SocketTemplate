#include "CszBitTorrent.h"

namespace Csz
{
	TorrentFile::TorrentFile():creation_date(0)
	{
#ifdef CszTest
		Csz::LI("construct Torrent File");
#endif
		name_data["announce"]=std::function<void(void*)>([this](void*T_data)
				{
					announce_list.push_back(std::string((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end()));
				});
		name_data["announce-list"]= std::function<void(void*)>([this](void* T_data)
				{
					announce_list.push_back(std::string((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end()));
				});
		name_data["comment"]=std::function<void(void*)>([this](void*T_data)
				{
					comment.assign((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end());
				});
		name_data["create by"]= std::function<void(void*)>([this](void*T_data)
				{
					create_by.assign((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end());
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
					infos.pieces.assign((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end());
				});

		name_data["infofileslength"]=std::function<void(void*)>([this](void*T_data)
				{
					if (infos.files.empty())
					{
						
						infos.files.emplace_back("",*((std::uint64_t*)T_data));
					}
					else if (infos.files.back().length== 0)
					{
						infos.files.back().length=*(std::uint64_t*)T_data;
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
						infos.files.emplace_back(std::string((*(std::string*)T_data).begin(),(*(std::string*)T_data).begin()),0);
					}
					else if (infos.files.back().file_path.empty()== 0)
					{
						infos.files.back().file_path.assign((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end());
					}
					else
					{
						infos.files.emplace_back(std::string((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end()),0);
					}
				});
		
		name_data["infoname"]= std::function<void(void*)>([this](void* T_data)
				{
					infos.name.assign((*((std::string*)T_data)).begin(),(*((std::string*)T_data)).end());
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
		Csz::LI("destructor Torrent File");
#endif
	}

	void TorrentFile::GetTrackInfo(Tracker* T_tracker)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (announce_list.empty())
		{
			Csz::ErrQuit("[Torrent File get track info]->failed ,announce is empty");
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
                //TODO 不退出,将所有正确结果读取出来
				Csz::ErrQuit("[Torrent File get track info]->failed,can't catch host or announce,string:%s",val.c_str());
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
					Csz::ErrQuit("[Torrent File get track info]->failed,can't found serv %s",val.c_str());
					return ;
				}
				//:54321
				data.serv.assign(val.substr(serv_flag+ 1,uri_flag- serv_flag- 1));
			}
			data.uri.assign(val.substr(uri_flag));
			//tcp set true.udp set false
			if ((host_flag= val.find("udp"))== val.npos)
				data.socket_tcp= true;
			else
				data.socket_tcp= false;
			T_tracker->SetTrackInfo(std::move(data));
		}
#ifdef CszTest
        Csz::LI("[Torrent File get track info]INFO:");
        COutInfo();
#endif
        return ;
	}

	std::string TorrentFile::GetHash(int32_t T_index) const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if ((T_index+ 1)* 20 > (int)infos.pieces.size() || T_index< 0)
		{
			Csz::ErrMsg("[Torrent File get hash]->failed, can't get hash,index= %d",T_index);
			return "";
		}
		return infos.pieces.substr(T_index* 20,20);
	}

	std::uint64_t TorrentFile::GetFileTotal()const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
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

	std::uint32_t TorrentFile::GetIndexTotal()const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (infos.pieces.empty())
		{
			Csz::ErrQuit("[Torrent File get index total]->failed, can't return index total,pieces is empty");
			return 0;
		}
		return infos.pieces.size()/ 20;
	}

	std::uint32_t TorrentFile::GetIndexBitTotal()const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (infos.pieces.empty())
		{
			Csz::ErrQuit("[Torrent File get index bit total]->failed,can't return index total,pieces is empty");
			return 0;
		}
		uint32_t ret=  infos.pieces.size()/ 20;
		return ret% 8== 0? ret/ 8 : ret/ 8 + 1;
	}

	//file name | begin | length
	//send piece,wirte piece
	std::vector<TorrentFile::FILEINFO> TorrentFile::GetFileName(int32_t T_index,int32_t T_begin,int32_t T_length) const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::vector<TorrentFile::FILEINFO> ret;
        if (T_index< 0)
        {
            Csz::ErrMsg("[Torrent File get file name]->failed,index< 0");
            return std::move(ret);
        }

		//1.single file
		if (infos.single)
		{
			//check begin+ length < sizeof(BT)
			TorrentFile::FILEINFO data;
			data.first= infos.name;
            if (T_index== GetIndexEnd())
            {
                data.second.first= T_index* infos.piece_length;
                data.second.second= GetFileTotal()- data.second.first;   
            }
            else
			{
			    data.second.first= T_index* infos.piece_length+ T_begin;
                data.second.second= T_length;
            }
			ret.emplace_back(std::move(data));
			return std::move(ret);
		}

		//2.multi file
		ret.resize(infos.files.size(),std::make_pair(infos.name+ "/",std::make_pair(0,0)));
		int64_t cur_total= (T_index)* infos.piece_length+ T_begin;
		int file_index= 0;
		for (auto start= infos.files.cbegin(),stop= infos.files.cend(); start< stop; ++start)
		{
			cur_total-= start->length;
			if (cur_total< 0)
			{
				ret[file_index].first.append(start->file_path);
				//begin
				ret[file_index].second.first= cur_total+ start->length;
				//sure require index in cur file
				if (cur_total+ T_length<= 0)
				{
					ret[file_index].second.second= T_length;
				}
				else
				{
					ret[file_index].second.second= start->length;
					++file_index;
					T_length+= cur_total;
					++start;
					while (T_length> 0 && start< stop)
					{
						ret[file_index].first.append(start->file_path);
						ret[file_index].second.first= 0;
						if (T_length- start->length> 0)
						{
							ret[file_index].second.second= start->length;
						}
						else
						{
							ret[file_index].second.second= T_length; 
						}
						T_length-= start->length;
						++file_index;
						++start;
					}
				}
				break;
			}
		}
		return std::move(ret);
    }

	std::vector<TorrentFile::FILEINFO> TorrentFile::GetFileName(int32_t T_index) const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		return GetFileName(T_index,0,infos.piece_length);
	}

    int32_t TorrentFile::GetPieceLength(int32_t T_index) const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        int32_t ret= 0;
        if (T_index< 0)
        {
            Csz::ErrMsg("[Torrent File get piece length]->failed,index< 0");
            return ret;   
        }
        int32_t index_count= infos.pieces.size()/ 20;
        //synch index
        --index_count;
		//1.check end index and total length<= piece length
        if (index_count== T_index && 0== index_count)
        {
            if (infos.single)
            {
                ret= infos.length;
            }
            else
            {
                for (auto &val : infos.files)
                {
                    ret+= val.length;
                }
            }
        }
		//2.normal index
        else
        {
			//end index and total length> piece length
            if (index_count== T_index)
            {   
                if (infos.single)
                {
                    ret= infos.length- index_count* infos.piece_length;
                }
                else
                {
                    for (const auto& val : infos.files)
                    {
                        ret+= val.length;
                    }
                    ret= ret- index_count* infos.piece_length;
                }
            }
            else
            {
                ret= infos.piece_length;
            }       
        }
        return ret;       
    }

    int32_t TorrentFile::GetPieceBit(int32_t T_index) const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        auto ret= GetPieceLength(T_index);
        if (ret% SLICESIZE== 0)
        {
            return ret/ SLICESIZE;
        }
        return ret/ SLICESIZE + 1;
    }
    
    std::pair<bool,int32_t> TorrentFile::CheckEndSlice(int32_t T_index) const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::pair<bool,int32_t> ret;
        ret.first= false;
        if (T_index== GetIndexEnd())
        {
            ret.first= true;
            auto piece_len= GetPieceLength(T_index);
            ret.second= piece_len% SLICESIZE;
        }
        return std::move(ret);
    }
    
    std::pair<bool,int32_t> TorrentFile::CheckEndSlice(int32_t T_index,int32_t T_begin) const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::pair<bool,int32_t> ret;
        ret.first= false;
        //1.check end piece
        if (T_index== GetIndexEnd())
        {
			ret.first= true;
            //2.end_piece length
            auto piece_len= GetPieceLength(T_index);
            if (piece_len< SLICESIZE)
            {
                //must start is 0
                if (0== T_begin)
                {
                    ret.second= piece_len;
                }
                else
                {
					ret.second= -1;
                    Csz::ErrMsg("[Torrent File check end slice]->failed,piece is end and piece len< slice size,but index!= 0");
                }
            }
            else if (0== T_begin% SLICESIZE)
            {
				if (T_begin+ SLICESIZE> piece_len)
					ret.second= piece_len% SLICESIZE;
				else
					ret.second= SLICESIZE;
            }
			else
			{
                Csz::ErrMsg("[Torrent File check end slice]->failed,piece is end and piece len> slice size,but begin%%SLICESIZE!= 0");
				ret.second= -1;
			}
        }
        return ret;
    }

    int32_t TorrentFile::GetIndexEnd() const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        return infos.pieces.size()/ 20 - 1;
    }

    int32_t TorrentFile::GetIndexEndLength() const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        uint64_t total= 0;
        if (infos.single)
        {
            total+= infos.length;
        }
        else
        {
            for (const auto& val : infos.files)
            {
                total+= val.length;
            }
        }
        return total- GetIndexEnd()* infos.piece_length;
    }

    int32_t TorrentFile::GetIndexNormalLength() const
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        return infos.piece_length;
    }

	void TorrentFile::COutInfo()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::string out_info;
		out_info.reserve(256);
        out_info.append("[Torrent File INFO]:");
		if (!announce_list.empty())
		{
			out_info.append("announce or announce_list:");
			for (const auto& val : announce_list)
			{
				out_info.append("->"+val);
			}
		}
		if (!comment.empty())
		{
			out_info.append("[comment:"+comment+"]");
		}
		if (creation_date)
		{
			out_info.append("[creation date:"+std::to_string(creation_date)+"]");
		}
		if (!create_by.empty())
		{
			out_info.append("[create by:"+create_by+ "]");
		}
		if (infos.piece_length)
		{
			out_info.append("[piece length:"+ std::to_string(infos.piece_length)+"]");
		}
		if (!infos.pieces.empty())
		{
			out_info.append("[pieces size:"+ std::to_string(infos.pieces.size())+"]");
		}
		if (infos.single)
		{
			out_info.append("[single mode]");
		}
		else
		{
			out_info.append("[multi mode]");
		}
		if (!infos.name.empty())
		{
			out_info.append("[name:"+infos.name+"]");
		}
		if (infos.length)
		{
			out_info.append("[length:"+std::to_string(infos.length)+"]");
		}
		if (!infos.files.empty())
		{
			for (const auto& val : infos.files)
			{
				if (!val.file_path.empty())
				{
					out_info.append("[file path:"+val.file_path+"]");
				}
				if (val.length)
				{
					out_info.append("[length:"+ std::to_string(val.length)+"]");
				}
			}
		}
		if (!out_info.empty())
			Csz::LI("%s",out_info.c_str());
	}
}
