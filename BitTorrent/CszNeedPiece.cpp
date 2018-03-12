#include <algorithm> //make_heap,push_heap,pop_heap
#include "CszBitTorrent.h"

namespace Csz
{
	void NeedPiece::PushNeed(const int32_t T_index,int T_socket,int T_id)
	{
		if (T_index< 0)
		{
			Csz::ErrMsg("[Need Piece push need]->failed,index< 0");
			return ;
		}

		//1.find index
		//iterator invalid,must lock
		auto start= index_queue.begin();
		auto stop= index_queue.end();
		while (start!= stop)
		{
			//1.1found
			if (T_index== (*start)->index)
			{
				((*start)->queue).emplace_back(std::make_pair(T_socket,T_id));
				break;
			}
		}
		//1.2not found
		if (start== stop)
		{
			std::shared_ptr<NeedPiece::DataType> data= std::make_shared<NeedPiece::DataType>();
			(data->queue).emplace_back(std::make_pair(T_socket,T_id));
			index_queue.push_back(std::move(data));
		}

		return ;
	}
	void NeedPiece::PushNeed(const std::vector<int32_t>* T_indexs,const int T_socket,int T_id)
	{
		if (nullptr== T_indexs || T_indexs->empty())
		{
			Csz::ErrMsg("[Need Piece push need]->failed,indexs is nullptr or empty");
			return ;
		}
		//iterator invalid,must lock
		for (const auto& val : *T_indexs)
		{
			PushNeed(val,T_socket,T_id);
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
		if (index_queue.empty())
		{
			Csz::ErrMsg("[Need Piece pop need]->failed,queue is empty");
			return ret;
		}
		//1.list sort
		index_queue.sort(NPComp);
		//TODO lock
		auto start= index_queue.begin();
		auto stop= index_queue.end();
		//TODO stop- start> 1
		while (start!= stop)
		{
			auto& result= *start;
			//2.check lock piece and socket status
			//2.1 check lock piece
			if (!(result->lock))
			{
				for (const auto& val : result->queue)
				{
					auto flag= id_queue.find(val.second);
					//2.2find socket id
					if (flag!= id_queue.end())
					{
						//2.2.1check status
						if (!(flag->second).peer_choke)
						{
							ret.second.emplace_back(val.first);
						}
					}
#ifdef CszTest
					else
					{
						Csz::LI("[Need Piece pop need]->failed,not found id=%d",val.second);
					}
#endif
				}	
				if (!ret.second.empty())
				{
					ret.first= result->index;
					result->lock= true;
					//index_queue.erase(start);
					break;
				}
			}
			++start;
		}
		return std::move(ret);
	}

	//support lock point piece
    std::vector<int> NeedPiece::PopPointNeed(int32_t T_index)
    {
        std::vector<int> ret;
        if (T_index< 0)
        {
            Csz::ErrMsg("[Need Piece pop point need]->failed,index < 0");
            return ret;
        }
        if (index_queue.empty())
        {
            Csz::ErrMsg("[Need Piece pop point need]->falied,index queue is empty");
        }
        //TODO lock
        //index vector
        for (auto& val : index_queue)
        {
            //1.find piece
            if (val->index== T_index)
            {
#ifdef CszTest
				//check lock piece status
				if (val->lock)
				{
					Csz::LI("[Need Piece pop point need]->not correct for index=%d",T_index);
				}
				else
				{
					Csz::LI("[NeedPiece pop point need]->correct for index=%d",T_index);
				}
#endif
#ifdef CszTest
				std::string out_info;
				out_info.reserve(64);
				out_info.append("Need Piece pop point need index=");
				out_info.append(std::to_string(T_index));
#endif
				for (auto& fd : val->queue)
				{
					//2.find id
					auto flag= id_queue.find(fd.second);
					if (flag!= id_queue.end())
					{
#ifdef CszTest
						out_info.append("socket=");
						out_info.append(std::to_string(fd.first));
						out_info.append(",status ");
						if ((flag->second).peer_choke)
							out_info.append("peer CHOKE");
						if ((flag->second).peer_interested)
							out_info.append("peer INTERESTED");
						else
							out_info.append("peer UNINTERESTED");
#endif
						//3.check socket status
						if (!(flag->second).peer_choke)
						{
							ret.emplace_back(fd.first);
#ifdef CszTest
							out_info.append("peer UNCHOKE");
#endif
						}
					}
#ifdef CszTest
					else
					{
						Csz::LI("[Need Piece pop point need]->failed,not found id=%d",fd.first);
						continue;
					}
#endif
                }
                break;
            }
        }
        return std::move(ret);
    }

	bool NeedPiece::Empty()const
	{
		return index_queue.empty() || id_queue.empty();
	}

	void NeedPiece::SocketMapId(int T_id)
	{
		//TODO lock
		id_queue.emplace(T_id,PeerStatus());
		return ;
	}
    
    void NeedPiece::ClearSocket(int T_id)
    {
        //TODO lock
        if (id_queue.find(T_id)!= id_queue.end())
        {
			//one method
            id_queue.erase(T_id);
			//two method
			//id_queue[T_id].peer_choke= 1;
			//id_queue[T_id].peer_interested= 0;
			//id_queue[T_id].am_choke= 1;
			//id_queue[T_id].am_interested= 0;
        }
    }

	void NeedPiece::AmChoke(int T_id)
	{
        auto p= RetSocketStatus(T_id);
        if (nullptr!= p)
        {
            (*p).am_choke= 1;
        }
		return ;
	}

	void NeedPiece::AmUnChoke(int T_id)
	{
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).am_choke= 0;
		}
		return ;
	}

	void NeedPiece::AmInterested(int T_id)
	{
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).am_interested= 1;
		}
		return ;
	}

	void NeedPiece::AmUnInterested(int T_id)
	{
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).am_interested= 0;
		}
		return ;
	}
    
	void NeedPiece::PrChoke(int T_id)
	{
        auto p= RetSocketStatus(T_id);
        if (nullptr!= p)
        {
            (*p).peer_choke= 1;
        }
		return ;
	}

	void NeedPiece::PrUnChoke(int T_id)
	{
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).peer_choke= 0;
			pop_cond.notify_one();
		}
		return ;
	}

	void NeedPiece::PrInterested(int T_id)
	{
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).peer_interested= 1;
		}
		return ;
	}

	void NeedPiece::PrUnInterested(int T_id)
	{
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).peer_interested= 0;
		}
		return ;
	}
	
    inline PeerStatus* NeedPiece::RetSocketStatus(int T_id)
    {
        //1.find socket
        auto flag= id_queue.find(T_id);
        if (flag!= id_queue.end())
        {
			return &(flag->second);
        }
#ifdef CszTest
		else
		{
			Csz::LI("[Need Piece ret socket status]->failed,not found socket id");
		}
#endif
        return nullptr;
    }

	void NeedPiece::ClearIndex(const int T_index)
	{
		if (T_index< 0)
		{
			Csz::ErrMsg("[Need Piece clear index]->failed,index< 0");
			return ;
		}
		//TODO lock
		auto start= index_queue.begin();
		auto stop= index_queue.end();
		while (start!= stop)
		{
			if(T_index== (*start)->index)
			{
				break;
			}
			++start;
		}
		if (start!= stop)
		{
			index_queue.erase(start);
		}
#ifdef CszTest
		else
		{
			Csz::LI("[Need Piece clear index]->failed,not found index");
		}
#endif 
		return ;
	}

	void NeedPiece::UnLockIndex(int T_index)
	{
		if (T_index< 0)
		{
			Csz::ErrMsg("[Need Piece unlock index]->failed,index< 0");
			return ;
		}
		//TODO lock
		auto start= index_queue.begin();
		auto stop= index_queue.end();
		while (start!= stop)
		{
			if(T_index== (*start)->index)
			{
				break;
			}
			++start;
		}
		if (start!= stop)
		{
			(*start)->lock= false;
		}
#ifdef CszTest
		else
		{
			Csz::LI("[Need Piece unlock index]->failed,not found index");
		}
#endif 
		return ;
	}

	void NeedPiece::Runner()
	{
		std::unique_lock<bthread::Mutex> guard(pop_mutex);
		//10s
		pop_cond.wait_for(guard,10000000);
		auto ret= PopNeed();
		if (ret.second.empty())
			return ;
		//TODO for
		Request request;
		request.SetParameter(ret.first,0,SLICESIZE);
		for (const auto& fd : ret.second)
		{
			auto code= send(fd,request.GetSendData(),request.GetDataSize(),0);
			if (code== request.GetDataSize())
			{
				break;
			}
#ifdef CszTest
			else
			{
				Csz::LI("[Need Piece runner]->send request failed,send length!=%d",request.GetDataSize());
			}
#endif
		}
		return ;
	}

	void NeedPiece::COutInfo()
	{
		for (auto& val : index_queue)
		{
			if (val!= nullptr)
			{
				std::string out_info;
				out_info.reserve(64);
				out_info.append("Need Piece INFO:index=");
				out_info.append(std::to_string(val->index));
				for (auto& fd : val->queue)
				{
                    auto flag= id_queue.find(fd.second);
                    if (flag!= id_queue.end())
                    {
						out_info.append(",socket="+std::to_string(fd.first));
                        if ((flag->second).peer_choke)
                        {
                            out_info.append(",status peer CHOKE");
                        }
                        else
                        {
						    out_info.append(",status peer UNCHOKE");
                        }
                        if ((flag->second).peer_interested)
                        {
                            out_info.append(",status peer INTERESTED");
                        }
                        else
                        {
                            out_info.append(",status peer UNINTERESTED");
                        }
                    }
                }
				if (!out_info.empty())
				{
					Csz::LI("%s",out_info.c_str());
				}
			}
		}
		return ;
	}
}
