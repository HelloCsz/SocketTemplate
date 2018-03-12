#include "CszBitTorrent.h"
#include <algorithm> //nth_element

namespace Csz
{
	void DownSpeed::AddTotal(const int T_socket,const uint32_t T_speed)
	{
		if (queue.empty())
		{
			Csz::ErrMsg("[Down Speed]->add total failed,queue is empty");
			return ;
		}
		for (auto& val : queue)
		{
			if (T_socket== val.socket)
			{
				val.total+= T_speed;
				break;
			}
		}
		return ;
	}

	void DownSpeed::AddTotal(std::vector<std::pair<int,uint32_t>>& T_queue)
	{
		for (const auto& val : T_queue)
		{
			AddTotal(val.first,val.second);
		}
		return ;
	}

	//have peer send interested
	std::vector<int> DownSpeed::RetSocket()
	{
#ifdef CszTest
        COutInfo();
#endif
		std::vector<int> ret;
		ret.reserve(5);
        int count= 0;
        for (const auto& val : queue)
        {
            //TODO check am choke
            if (val.status.peer_interested)
            {
                ret.emplace_back(val.socket);
                ++count;
            }
            if (5== count)
                break;
        }
		return std::move(ret);
	}

	bool DownSpeed::CheckSocket(const int T_socket)
	{
        auto check= RetSocket();
        for (const auto& val : check)
        {
            if (T_socket== val)
            {
                return true;
            }
        }
		return false;
	}
    
    void DownSpeed::ClearSocket(int T_socket)
    {
        auto flag= queue.cbegin();
        auto stop= queue.cend();
        for (; flag!= stop; ++flag)
        {
            if (flag->socket== T_socket)
                break;
        }       
        if (flag!= stop)
        {
            queue.erase(flag);
        }
        return ;
    }

	//hash table find status
	void DownSpeed::AmChoke(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_choke= 1;
				break;
			}
		}
		return ;
	}

	void DownSpeed::AmUnChoke(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_choke= 0;
				break;
			}
		}
		return ;
	}

	void DownSpeed::AmInterested(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_interested= 1;
				break;
			}
		}
		return ;
	}

	void DownSpeed::AmUnInterested(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_interested= 0;
				break;
			}
		}
		return ;
	}

	void DownSpeed::PrChoke(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_choke= 1;
				break;
			}
		}
		return ;
	}

	void DownSpeed::PrUnChoke(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_choke= 0;
				break;
			}
		}
		return ;
	}

	void DownSpeed::PrInterested(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_interested= 1;
				break;
			}
		}
		return ;
	}

	void DownSpeed::PrUnInterested(int T_socket)
	{
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_interested= 0;
				break;
			}
		}
		return ;
	}

	void DownSpeed::CalculateSpeed()
	{
		queue.sort(DSComp);
		//send
		auto peer_manager= PeerManager::GetInstance();
		int unchoke_count= 4;
		//TODO die lock(may be),PeerManager call DownSpeed,alter socket status
		for (auto & val : queue)
		{
			if (val.status.peer_interested && unchoke_count> 0)
			{
				--unchoke_count;
				peer_manager->AmUnChoke(val.socket);
			}
			else
			{
				peer_manager->AmChoke(val.socket);
			}
		}
		return ;
	}

	void DownSpeed::COutInfo()
	{
		std::string info_data;
		info_data.reserve(256);
		queue.sort(DSComp);
		//std::sort_heap(queue.begin(),queue.end(),DSComp);
		for (const auto& val : queue)
		{
			info_data.append(std::to_string(val.socket)+":"+std::to_string(val.total)+",status ");
            if (val.status.am_choke)
            {
                info_data.append("am CHOKE,");
            }
            else
            {
                info_data.append("am UNCHOKE,");
            }
            if (val.status.peer_interested)
            {
                info_data.append("peer INTERESTED");
            }
            else 
            {
                info_data.append("peer UNINTERESTED");
            }
		    if (!info_data.empty())
			{ 
                Csz::LI("[Down Speed INFO]:%s",info_data.c_str());
            }
            info_data.clear();
		}
		return ;
	}
}
