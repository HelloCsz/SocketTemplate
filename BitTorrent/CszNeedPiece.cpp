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

    //TODO peer_UNCHOKE && peer_INTERESTED || peer_UNINTERESTED(!!)
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
						if (!(step_2->second).peer_choke)
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
        std::string out_info;
        out_info.reserve(64);
        out_info.append("Need Piece pop point need index=");
        out_into.append(std::to_string(T_index));
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
                if (val.second!= nullptr)
                {
                    for (auto& fd : *(val.second))
                    {
                        auto step1= socket_cur.find(fd);
                        if (step1!= socket_cur.end())
                        {
                            //3.socket queue
                            auto step2= socket_queue.find(step1->second);
                            if (step2!= socket_queue.end())
                            {
#ifdef CszTest
                                out_info.append("socket=");
                                out_info.append(std::to_string(step1.first));
                                out_info.append(",status ");
                                if ((step2->second).peer_choke)
                                    out_info.append("peer CHOKE");
                                if ((step2->second).peer_interested)
                                    out_info.append("peer INTERESTED");
                                else
                                    out_info.append("peer UNINTERESTED");
#endif
                                //4.check status
                                if (!(step2->second).peer_choke)
                                {
#ifdef CszTest
                                    out_info.append("peer UNCHOKE");
#endif
                                    ret.emplace_back(fd);
                                }
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

	void NeedPiece::NPChoke(int T_socket)
	{
        auto p= RetSocketStatus(T_socket);
        if (nullptr!= p)
        {
            (*p).
        }
		return ;
	}

	void NeedPiece::NPUnChoke(int T_socket)
	{
		return ;
	}

	void NeedPiece::NPInterested(int T_socket)
	{
		return ;
	}

	void NeedPiece::NPUnInterested(int T_socket)
	{
		return ;
	}
    
    inline int* NeedPiece::RetSocketStatus(int T_socket)
    {
        //1.find socket
        auto step1= socket_cur.find(T_socket);
        if (step1!= socket_cur.end())
        {
            //2.find id
            auto step2= socket_queue.find(step1->second);
            if (step2!= socket_queue.end())
            {
                //3.return status
                return &(step2->second);
            }
        }
        return nullptr;
    }

	void NeedPiece::COutInfo()
	{
		for (auto& val : queue)
		{
			if (val.second!= nullptr)
			{
				std::string out_info;
				out_info.reserve(64);
				out_info.append("Need Piece INFO:index=");
				out_info.append(std::to_string(val.first));
				for (auto& fd : *val.second)
				{
					out_info.append(",socket="+std::to_string(fd));
                    auto step1= socket_cur.find(fd);
                    if (step1!= socket_cur.end())
                    {
                        auto step2= socket_queue.find(step1->second);
                        if (step2!= socket_queue.end())
                        {
                            if (step2->second.peer_choke)
                            {
                                out_info.append(",status peer CHOKE");
                            }
                            else
                            {
                                out_info.append(",status peer UNCHOKE");
                            }
                            if (step2->second.peer_interested)
                            {
                                out_info.append(",status peer INTERESTED");
                            }
                            else
                            {
                                out_info.append(",status peer UNINTERESTED");
                            }
                        }
                    }
				}
				Csz::LI("%s",out_info.c_str());
			}
		}
		return ;
	}
}
