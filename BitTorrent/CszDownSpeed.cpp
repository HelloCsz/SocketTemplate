#include "CszBitTorrent.h"
#include <algorithm> //nth_element

//brpc
#include <butil/time.h> //seconds_from_now
//#include <bthread/unstable.h> //bthread_timer_add,bthread_timer_del

namespace Csz
{
	void DownSpeed::AddTotal(const int T_socket,const uint32_t T_speed)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //RAII LOCK
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed]->failed,write lock failed");
            return ;
        }

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
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::AddTotal(std::vector<std::pair<int,uint32_t>>& T_queue)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
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
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::vector<int> ret;
		ret.reserve(5);
        int count= 0;
        //TODO RAII lock
        if (0!= pthread_rwlock_rdlock(&lock))
        {
            Csz::ErrMsg("[Down Speed ret socket]->failed,read lock failed");
            return ret;
        }
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
        pthread_rwlock_unlock(&lock);
        //unlock
		return std::move(ret);
	}

	bool DownSpeed::CheckSocket(const int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
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
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed clear socket]->failed,write lock failed");
            return ;
        }
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
        pthread_rwlock_unlock(&lock);
        //unlock
        return ;
    }

	//hash table find status
	void DownSpeed::AmChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed am choke]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_choke= 1;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::AmUnChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed am unchoke]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_choke= 0;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::AmInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed am interested]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_interested= 1;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::AmUnInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed am uninterested]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.am_interested= 0;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::PrChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed pr choke]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_choke= 1;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::PrUnChoke(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed pr unchoke]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_choke= 0;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::PrInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed pr interested]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_interested= 1;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
        //unlock
		return ;
	}

	void DownSpeed::PrUnInterested(int T_socket)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed pr uninterested]->failed,write lock failed");
            return ;
        }
		for (auto& val : queue)
		{
			if (val.socket== T_socket)
			{
				val.status.peer_interested= 0;
				break;
			}
		}
        pthread_rwlock_unlock(&lock);
		return ;
	}

	void DownSpeed::CalculateSpeed()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //lock
        if (0!= pthread_rwlock_wrlock(&lock))
        {
            Csz::ErrMsg("[Down Speed calculate speed]->failed,write lock failed");
            return ;
        }     
		queue.sort(DSComp);
#ifdef CszTest
        Csz::LI("[Down Speed calculate speed]INFO:");
        COutInfo();
#endif
        uint32_t speed= 0;
		//send
		auto peer_manager= PeerManager::GetInstance();
		int unchoke_count= 4;
        std::vector<std::pair<bool,int>> result;
        result.reserve(queue.size());
		//TODO die lock(may be),PeerManager call DownSpeed,alter socket status
		for (auto & val : queue)
		{
            speed+= val.total;
            //bug
            //interested not have 4 num
			if (val.status.peer_interested && unchoke_count> 0)
			{
				--unchoke_count;
                result.emplace_back(std::make_pair(false,val.socket));
			}
			else
			{
                //TODO delte false
                result.emplace_back(std::make_pair(false,val.socket));
			}
            val.total= 0;
		}
        pthread_rwlock_unlock(&lock);
        //unlock

        for (const auto& val : result)
        {
            if (val.first)
            {
                peer_manager->AmChoke(val.second);
            }
            else
            {
                peer_manager->AmUnChoke(val.second);
            }
        }
        std::cout<<"speed="<<speed/ 1024<<"kb"<<"\n";
		return ;
	}

	void DownSpeed::_CalculateSpeed(void* T_this)
	{
		DownSpeed* cur_this= static_cast<DownSpeed*>(T_this);
		cur_this->CalculateSpeed();
		//10s
		auto until_time= butil::seconds_from_now(10);
		auto code= bthread_timer_add(&cur_this->id,until_time,&DownSpeed::_CalculateSpeed,T_this);
		if (code!= 0)
		{
			Csz::ErrMsg("[%s->%d]->failed,bthread timer add failed code=%d",__func__,__LINE__,code);
		}
		return ;
	}

	void DownSpeed::Clear()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		auto code= bthread_timer_del(id);
		if (code!= 0)
		{
			Csz::ErrMsg("[%s->%s->%d]->failed,calculate speed still running or einval,code=%d",__FILE__,__func__,__LINE__,code);
		}
		pthread_rwlock_destroy(&lock);
		queue.clear();
		return ;
	}

	void DownSpeed::COutInfo()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
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
