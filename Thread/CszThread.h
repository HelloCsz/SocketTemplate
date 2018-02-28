#ifndef CszTHREAD_HPP
#define CszTHREAD_HPP

#include <thread/thread.h> //bthread_start_background
#include <thread/condition_variable.h> //ConditionVariable
#include <thread/mutex.h> //Mutex
#include <algorithm>
#include <vector>
#include "../CszNonCopyAble.hpp"
namespace Csz
{
	//semi-sync and semi-async
	template <class Parameter,int TASKNUM>
	class TaskQueue : public NoncopyAble
	{
		private:
			struct Data
			{
				Data():priority(0),parameter(nullptr){}
				int priority;
				Parameter* parameter;
				std::function<void(void*)> func;
			};
		private:
			bool stop;
			bthread::ConditionVariable push_cond;
			bthread::ConditionVariable pop_cond;
			bthread::Mutex queue_mutex;
			std::vector<Data> task_queue;
		public:
			using Type= std::pair<std::function<void(Parameter*)>,Parameter*>;
			TaskQueue();
			~TaskQueue();
			void Push(Type* T_task);
			void Push(std::vector<Type>* T_tasks);
			bool Pop(Type* T_task);
			bool Pop(std::vector<Type>* T_tasks);
			//TODO lock
			bool Empty()const {return task_queue.empty();}
			void Stop();
		private:
			void _Push(Type* T_task);
			bool _Pop(Type* T_task);
	};

	template<class Parameter,int TASKNUM>
	class BthreadPool
	{
		private:
			std::vector<bthread_t> pool;
			TaskQueue<Parameter,TASKNUM> task_list;
		public:
			BthreadPool()= default;
			~BthreadPool();
            void Init(int T_num);
			void SetConcurrency(int T_num);
			int GetConcurrency();
			void Push(TaskQueue::Type* T_task);
			bool Pop(TaskQueue::Type* T_task);
			static void* Run(void*);
			void Stop();
	};
}
#endif //CszTHREAD_HPP

//impl
#include "CszTaskQueue.hpp"
#include "CszBthreadPool.hpp"
