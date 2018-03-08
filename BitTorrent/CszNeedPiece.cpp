#include <algorithm> //make_heap,push_heap,pop_heap
#include "CszBitTorrent.h"

namespace Csz
{
	void NeedPiece::PushNeed(const int32_t T_index,int T_socket)
	{
		if (T_index< 0)
		{
			Csz::ErrMsg("NeedPiece can't push,index< 0");
			return ;
		}
		//iterator invalid,must lock
		auto start= queue.begin();
		auto stop= queue.end();
		while (start< stop)
		{
			//1.found
			if (T_index== start->first)
			{
				start->second->emplace_back(T_socket);
				break;
			}
		}
		//2.not found
		if (start== stop)
		{
			std::shared_ptr<std::vector<int>> data(new std::vector<int>);
			data->emplace_back(T_socket);
			queue.emplace_back(std::make_pair(T_index,data));
			std::push_heap(queue.begin(),queue.end(),NPComp);
		}
		else
		{
			//found
			std::make_heap(queue.begin(),queue.end(),NPComp);
		}
		return ;
	}
	void NeedPiece::PushNeed(const std::vector<int32_t>* T_indexs,const int T_socket)
	{
		if (nullptr== T_indexs || T_indexs->empty())
		{
			Csz::ErrMsg("NeedPiece can't push,indexs is nullptr or empty");
			return ;
		}
		//iterator invalid,must lock
		for (const auto& val : *T_indexs)
		{
			PushNeed(val,T_socket);
		}
		return ;
	}

	std::pair<int32_t,std::vector<int>> NeedPiece::PopNeed()
	{
#ifdef CszTest
        COutInfo();
#endif
		std::pair<int32_t,std::vector<int>> ret;
		if (queue.empty())
		{
			Csz::ErrMsg("Need Piece,queue is empty");
			return ret;
		}
		//TODO lock
		auto start= queue.begin();
		auto stop= queue.end();
		while (start< stop)
		{
			std::pop_heap(start,stop,NPComp);
			auto& result= *stop;
			//check socket status
			for (const auto& val : *(result.second))
			{
				auto step_1= socket_cur.find(val);
				//find socket map id
				if (step_1== socket_cur.end())
				{
					Csz::ErrMsg("Need Piece socket map error,not found %d",val);
					return std::move(ret);
				}
				else
				{
					auto step_2= socket_queue.find(step_1->second);
					//find id map socket status
					if (step_2== socket_queue.end())
					{
						Csz::ErrMsg("Need Piece socket map error,not found %d map id",val);
						return std::move(ret);
					}
					else
					{
						//unchoke status
						if (step_2->second & UNCHOKE)
						{
							ret.second.emplace_back(val);
						}
					}
				}
			}
			if (!ret.second.empty())
			{
				ret.first= result.first;
				queue.erase(stop);
				break;
			}
			--stop;
		}
		//TODO async
		std::make_heap(queue.begin(),queue.end());
		return std::move(ret);
	}

    std::vector<int> NeedPiece::PopPointNeed(int32_t T_index)
    {
#ifdef CszTest
        COutInfo();
#endif
        std::vector<int> ret;
        if (T_index< 0)
        {
            Csz::ErrMsg("Need Piece pop point need,index < 0");
            return ret;
        }
        if (queue.empty())
        {
            Csz::ErrMsg("Need Piece pop point need,queue is empty");
        }
        //TODO lock
        //index vector
        for (auto& val : queue)
        {
            //1.find 
            if (val.first== T_index)
            {
                //2.cur socket vector
                for (auto& fd : *(val.second))
                {
                    auto step1= socket_cur.find(fd);
                    if (step1!= socket_cur.end())
                    {
                        //3.socket queue
                        auto step2= socket_queue.find(step1->second);
                        if (step2!= socket_queue.end())
                        {
                            //4.check status
                            if (step2->second & UNCHOKE)
                            {
                                ret.emplace_back(fd);
                            }
                        }
                    }
                }
                break;
            }
        }
        return std::move(ret);
    }

	bool NeedPiece::Empty()const
	{
		return queue.empty() || socket_cur.empty();
	}

	void NeedPiece::SocketMapId(std::vector<int>& T_ret)
	{
		//TODO lock
		for (const auto& val : T_ret)
		{
			int id= cur_id++;
			socket_cur.emplace(val,id);
			socket_queue.emplace(id,0);
		}
		return ;
	}
    
    void NeedPiece::ClearSocket(int T_socket)
    {
        //TODO lock
        if (socket_cur.find(T_socket)!= socket_cur.end())
        {
            socket_cur.erase(T_socket);
        }
    }

	inline bool NeedPiece::_SetSocketStatus(int T_socket,char T_status)
	{
		auto step_1= socket_cur.find(T_socket);
		if (step_1== socket_cur.end())
		{
			return false;
		}
		auto step_2= socket_queue.find(step_1->second);
		if (step_2== socket_queue.end())
		{
			Csz::ErrMsg("Need Piece set socket status ,not found socket map id num");
			return false;
		}
		step_2->second|= T_status;
		return true;
	}

	inline bool NeedPiece::_ClearSocketStatus(int T_socket,char T_status)
	{
		auto step_1= socket_cur.find(T_socket);
		if (step_1== socket_cur.end())
		{
			return false;
		}
		auto step_2= socket_queue.find(step_1->second);
		if (step_2== socket_queue.end())
		{
			Csz::ErrMsg("Need Piece clear socket status ,not found socket map id num");
			return false;
		}
		step_2->second&= (~T_status);
		return true;
	}

	void NeedPiece::NPChoke(int T_socket)
	{
		if (_SetSocketStatus(T_socket,CHOKE))
		{
			_ClearSocketStatus(T_socket,UNCHOKE);
		}
		return ;
	}

	void NeedPiece::NPUnChoke(int T_socket)
	{
		if (_SetSocketStatus(T_socket,UNCHOKE))
		{
			_ClearSocketStatus(T_socket,CHOKE);
		}
		return ;
	}

	void NeedPiece::NPInterested(int T_socket)
	{
		if (_SetSocketStatus(T_socket,INTERESTED))
		{
			_ClearSocketStatus(T_socket,UNINTERESTED);
		}
		return ;
	}

	void NeedPiece::NPUnInterested(int T_socket)
	{
		if (_SetSocketStatus(T_socket,UNINTERESTED))
		{
			_ClearSocketStatus(T_socket,INTERESTED);
		}
		return ;
	}

	void NeedPiece::COutInfo()
	{
		for (auto& val : queue)
		{
			if (val.second!= nullptr)
			{
				std::string out_info;
				out_info.reserve(64);
				out_info.append("Need Piece info:index=");
				out_info.append(std::to_string(val.first));
				for (auto& fd : *val.second)
				{
					out_info.append(",socket="+std::to_string(fd));
					if (socket_queue[fd]& CHOKE)
					{
						out_info.append("->CHOKE");
					}
					else
					{
						out_info.append("->UNCHOKE");
					}
					if (socket_queue[fd] & INTERESTED)
					{
						out_info.append("->INTERESTED");
					}
					else
					{
						out_info.append("->UNINTERESTED");
					}
				}
				Csz::LI("%s",out_info.c_str());
			}
		}
		return ;
	}
}
