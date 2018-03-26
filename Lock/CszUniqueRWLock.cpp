#include "CszLock.h"

namespace Csz
{
	explicit UniqueRWLock::UniqueRWLock(pthread_rwlock_t& T_mutex,bool T_rdlock= true) : pmutex(&T_mutex),lock(false)
	{
		if (T_rdlock)
		{
			if (0!= pthread_rwlock_rdlock(pmutex))
			{
				Csz::ErrMsg("[%s->%d]->failed,read lock failed",__func__,__LINE__);
			}
			else
			{
				lock= true;
			}
		}
		else
		{
			if (0!= pthread_rwlock_wrlock(pmutex))
			{
				Csz::ErrMsg("[%s->%d]->failed,write lock failed",__func__,__LINE__);
			}
			else
			{
				lock= true;
			}
		}
	}

	void UniqueRWLock::RdLock()
	{
		if (lock)
		{
			Csz::ErrMsg("[%s->%d]->failed,detected deadlock issue",__func__,__LINE__);
			return ;
		}
		else
		{
			if (0!= pthread_rwlock_rdlock(pmutex))
			{
				Csz::ErrMsg("[%s->%d]->failed,read lock failed",__func__,__LINE__);
			}
			else
			{
				lock= true;
			}
		}
		return ;
	}

	void UniqueRWLock::WrLock()
	{
		if (lock)
		{
			Csz::ErrMsg("[%s->%d]->failed,detected deadlock issue",__func__,__LINE__);
			return ;
		}
		else
		{
			if (0!= pthread_rwlock_wrlock(pmutex))
			{
				Csz::ErrMsg("[%s->%d]->failed,write lock failed",__func__,__LINE__);
			}
			else
			{
				lock= true;
			}
		}
		return ;
	}


	void UniqueRWLock::UnLock()
	{
		if (!lock)
		{
			Csz::ErrMsg("[%s->%d]->failed,detected double free lock",__func__,__LINE__);
			return ;
		}
		else
		{
			if (0!= pthread_rwlock_unlock(pmutex))
			{
				Csz::ErrMsg("[%s->%d]->failed,unlock failed",__func__,__LINE__);
			}
			else
			{
				lock= false;
			}
		}
		return ;
	}
}
