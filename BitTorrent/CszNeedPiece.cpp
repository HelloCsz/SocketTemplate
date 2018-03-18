//brpc
#include <bthread/bthread.h>
#include <bthread/mutex.h>
#include <algorithm> //make_heap,push_heap,pop_heap
#include "CszBitTorrent.h"

namespace Csz
{
	void NeedPiece::PushNeed(const int32_t T_index,int T_socket,int T_id)
	{
#ifdef CszTest
        //Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (T_index< 0)
		{
			Csz::ErrMsg("[Need Piece push need]->failed,index< 0");
			return ;
		}

		//1.find index
		//iterator invalid,must lock
		std::lock_guard<bthread::Mutex> guard(index_lock);
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
            ++start;
		}
		//1.2not found
		if (start== stop)
		{
			std::shared_ptr<NeedPiece::DataType> data= std::make_shared<NeedPiece::DataType>();
            //fix bug,not write index
            data->index= T_index;
			(data->queue).emplace_back(std::make_pair(T_socket,T_id));
			index_queue.push_back(std::move(data));
		}

		return ;
	}

	void NeedPiece::PushNeed(const std::vector<int32_t>* T_indexs,const int T_socket,int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d->socket=%d->id=%d]",__FILE__,__func__,__LINE__,T_socket,T_id);
#endif
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
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::pair<int32_t,std::vector<int>> ret;
		//TODO lock
		std::lock_guard<bthread::Mutex> guard(index_lock);
		if (index_queue.empty())
		{
			Csz::ErrMsg("[Need Piece pop need]->failed,queue is empty");
			return ret;
		}
		//1.list sort
		index_queue.sort(NPComp);
		auto start= index_queue.begin();
		auto stop= index_queue.end();
		//TODO stop- start> 1
		while (start!= stop)
		{
			bool judge= false;
			auto& result= *start;
			//2.check lock piece and socket status
			//2.1 check lock piece
			if (result->index_status & 0x01)
			{
				if (time(NULL)- result->expire>= EXPIRETIME)
				{
					judge= true;
				}
			}
			else
			{
				judge= true;
			}

			if (judge)
			{
				for (const auto& val : result->queue)
				{
                    //lock
                    if (0!= pthread_rwlock_rdlock(&id_lock))
                    {
                        Csz::ErrMsg("[Need Piece pop need]->failed,read lock failed");
                        continue;
                    }
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
                    pthread_rwlock_unlock(&id_lock);
                    //unlock
				}	
				if (!ret.second.empty())
				{
                    //TODO read
                    result->index_status= 0x01;
					result->expire= time(NULL);
					ret.first= result->index;
					//index_queue.erase(start);
					break;
				}
			}
			++start;
		}
#ifdef CszTest
        //Csz::LI("[Need Piece pop need]INFO:");
        //COutInfo();
#endif
		return std::move(ret);
	}

	//support lock point piece
    std::vector<int> NeedPiece::PopPointNeed(int32_t T_index)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //TODO lock
        std::lock_guard<bthread::Mutex> guard(index_lock);
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

        //index vector
        for (auto& val : index_queue)
        {
            //1.find piece
            if (val->index== T_index)
            {
#ifdef CszTest
				//check lock piece status
				if ((val->index_status & 0x02) && (val->index_status & 0x01))
				{
					Csz::LI("[Need Piece pop point need]->correct for index=%d",T_index);
				}
				else
				{
					Csz::LI("[Need Piece pop point need]->not correct for index=%d",T_index);
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
                    //lock
                    if (0!=pthread_rwlock_rdlock(&id_lock))
                    {
                        Csz::ErrMsg("[Need Piece pop point need]->failed,read lock failed");
                        continue;
                    }
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
					}
#endif
                    pthread_rwlock_unlock(&id_lock);
                }
                break;
            }
        }
        return std::move(ret);
    }

	bool NeedPiece::Empty()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        std::lock_guard<bthread::Mutex> guard(index_lock);
        pthread_rwlock_rdlock(&id_lock);
		return index_queue.empty() || id_queue.empty();
	}

	void NeedPiece::SocketMapId(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		//TODO lock
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece socket map id]->failed,write lock failed");
            return ;
        }
		id_queue.emplace(T_id,PeerStatus());
        pthread_rwlock_unlock(&id_lock);
        //unlock
		return ;
	}
    
    void NeedPiece::ClearSocket(int T_id)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //TODO lock
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece clear socket]->failed,write lock failed");
            return ;
        }
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
        pthread_rwlock_unlock(&id_lock);
        //unlock
        return ;
    }

	void NeedPiece::AmChoke(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece am choke]->failed,write lock failed");
            return ;
        }
        auto p= RetSocketStatus(T_id);
        if (nullptr!= p)
        {
            (*p).am_choke= 1;
        }
        pthread_rwlock_unlock(&id_lock);
		return ;
	}

	void NeedPiece::AmUnChoke(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece am unchoke]->failed,write lock failed");
            return ;
        }
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).am_choke= 0;
		}
        pthread_rwlock_unlock(&id_lock);
		return ;
	}

	void NeedPiece::AmInterested(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece am interested]->failed,write lock failed");
            return ;
        }
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).am_interested= 1;
		}
        pthread_rwlock_unlock(&id_lock);
		return ;
	}

	void NeedPiece::AmUnInterested(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece am uninterested]->failed,write lock failed");
            return ;
        }
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).am_interested= 0;
		}
        pthread_rwlock_unlock(&id_lock);
		return ;
	}
    
	void NeedPiece::PrChoke(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece pr choke]->failed,write lock failed");
            return ;
        }
        auto p= RetSocketStatus(T_id);
        if (nullptr!= p)
        {
            (*p).peer_choke= 1;
        }
        pthread_rwlock_unlock(&id_lock);
		return ;
	}

	void NeedPiece::PrUnChoke(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece pr unchoke]->failed,write lock failed");
            return ;
        }
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).peer_choke= 0;
            pthread_rwlock_unlock(&id_lock);
            SendReq();
/*
            bthread_t tid;
            if (bthread_start_background(&tid,NULL,ASendReq,static_cast<void*>(this))!= 0)
            {
                SendReq();
                Csz::ErrMsg("[Need Piece pr unchoke]->failed,create bthread run send request");
            }   
			//pop_cond.notify_one();
*/
		}
        else
        {
            pthread_rwlock_unlock(&id_lock);
        }
		return ;
	}

	void NeedPiece::PrInterested(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece pr interested]->failed,write lock failed");
            return ;
        }
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).peer_interested= 1;
		}
        pthread_rwlock_unlock(&id_lock);
		return ;
	}

	void NeedPiece::PrUnInterested(int T_id)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (0!= pthread_rwlock_wrlock(&id_lock))
        {
            Csz::ErrMsg("[Need Piece pr uninterested]->failed,write lock failed");
            return ;
        }
		auto p= RetSocketStatus(T_id);
		if (nullptr!= p)
		{
			(*p).peer_interested= 0;
		}
        pthread_rwlock_unlock(&id_lock);
		return ;
	}
	
    inline PeerStatus* NeedPiece::RetSocketStatus(int T_id)
    {
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
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
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (T_index< 0)
		{
			Csz::ErrMsg("[Need Piece clear index]->failed,index< 0");
			return ;
		}
		//TODO lock
		std::lock_guard<bthread::Mutex> guard(index_lock);
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
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (T_index< 0)
		{
			Csz::ErrMsg("[Need Piece unlock index]->failed,index< 0");
			return ;
		}
		//TODO lock
		std::lock_guard<bthread::Mutex> guard(index_lock);
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
			(*start)->index_status= 0;
		}
#ifdef CszTest
		else
		{
			Csz::LI("[Need Piece unlock index]->failed,not found index");
		}
#endif 
		return ;
	}

    void NeedPiece::SendReq()
    {    
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		auto ret= PopNeed();
		if (ret.second.empty())
			return ;
        auto peer_manager= PeerManager::GetInstance();
		Request request;
		request.SetParameter(ret.first,0,SLICESIZE);
		for (const auto& fd : ret.second)
		{
#ifdef CszTest
            Csz::LI("[Need Piece send req]->index=%d,socket=%d",ret.first,fd);
#endif
            //socket mutex
            auto mutex= peer_manager->GetSocketMutex(fd);
            if (nullptr== mutex)
            {
                continue;
            }
            //TODO all socket
            std::lock_guard<bthread::Mutex> guard(*mutex);
			auto code= send(fd,request.GetSendData(),request.GetDataSize(),0);
			if (code== request.GetDataSize())
			{
                ;
				//break;
			}
#ifdef CszTest
			else
			{
				Csz::LI("[Need Piece send req]->send request failed,send length!=%d",request.GetDataSize());
			}
#endif
		}
		return ;
    }

    bool NeedPiece::SetIndexW(int32_t T_index)
    {
        bool ret= false;
        if (T_index< 0)
        {
            Csz::ErrMsg("[Need Piece ret index status]->failed,index< 0");
            return ret;
        }
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		//TODO lock
		std::lock_guard<bthread::Mutex> guard(index_lock);
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
			if((*start)->index_status & 0x02)
            {
                ;
            }
            else
            {
                (*start)->index_status|= 0x02;
                ret= true;
            }
		}
		else
		{
			Csz::ErrMsg("[Need Piece unlock index]->failed,not found index");
		}
		return ret;
    }

/*
	void NeedPiece::Runner()
	{
		std::lock_guard<bthread::Mutex> guard(pop_mutex);
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
            //socket mutex
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
*/

	void NeedPiece::COutInfo()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
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
					Csz::LI("[Need Piece INFO]:%s",out_info.c_str());
				}
			}
		}
		return ;
	}
}
