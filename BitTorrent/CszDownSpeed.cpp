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
		if (queue.size()> 4)
		{
			//并非绝对排序
			std::nth_element(queue.begin(),queue.begin()+ 4,queue.end(),DSComp);
		}
		std::vector<int> ret;
		ret.reserve(4);
		for (auto start= queue.cbegin(),stop= queue.cbegin()+ 4,end= queue.cend(); start< stop && start< end; ++start)
		{
			ret.emplace_back(start->second);
		}
		return std::move(ret);
	}
}
