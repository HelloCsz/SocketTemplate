#ifndef CszLOCK_H
#define CszLOCK_H
#include <pthread.h>
#include "../CszNonCopyAble.hpp"
namespace Csz
{
	//support RAII
	class UniqueRWLock : public NonCopyAble
	{
		private:
			bool lock;
			pthread_rwlock_t* pmutex;
		public:
			explicit UniqueRWLock(pthread_rwlock_t& T_mutex,bool T_rdlock= true);
			void RdLock();
			void WrLock();
			void UnLock();
			~UniqueRWLock()
			{
				if (lock)
				{
					UnLock();
				}
			}
	}
}

#endif //CszLOCK_H
