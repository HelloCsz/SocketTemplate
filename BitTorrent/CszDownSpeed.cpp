#include "CszBitTorrent.h"
#include <algorithm> //nth_element

namespace Csz
{
	void DownSpeed::AddTotal(const int T_socket,const uint32_t T_speed)
	{
		if (queue.empty())
		{
			Csz::ErrMsg("DownSpeed can't add total,queue is empty");
			return ;
		}
		for (auto& val : queue)
		{
			if (T_socket== val.first)
			{
				val.second+= T_speed;
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
		auto stop= queue.cend();
		if (queue.size()> 4)
		{
			//并非绝对排序
			std::nth_element(queue.begin(),queue.begin()+ 4,queue.end(),DSComp);
			stop= queue.cbegin()+ 4;
		}
		std::vector<int> ret;
		ret.reserve(4);
		for (auto start= queue.cbegin(); start< stop; ++start)
		{
			ret.emplace_back(start->first);
		}
		return std::move(ret);
	}

	bool DownSpeed::CheckSocket(const int T_socket)
	{
		auto stop= queue.cend();
		if (queue.size()> 4)
		{
			std::nth_element(queue.begin(),queue.begin()+ 4,queue.end(),DSComp);
			stop= queue.cbegin()+ 4;
		}
		for (auto start= queue.cbegin(); start< stop; ++start)
		{
			if (T_socket== start->first)
				return true;
		}
		return false;
	}
    
    void DownSpeed::ClearSocket(int T_socket)
    {
        auto flag= queue.cbegin();
        auto stop= queue.cend();
        for (; flag< stop; ++flag)
        {
            if (flag->first== T_socket)
                break;
        }       
        if (flag!= stop)
        {
            queue.erase(flag);
        }
        return ;
    }

	void DownSpeed::COutInfo()
	{
		std::string info_data;
		info_data.reserve(256);
		std::sort_heap(queue.begin(),queue.end(),DSComp);
		for (const auto& val : queue)
		{
			info_data.append(std::to_string(val.first)+":"+std::to_string(val.second)+";");
		}
		Csz::LI("Down Speed sort:%s",info_data.c_str());
		return ;
	}
}
