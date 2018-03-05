#ifndef CszTASKQUEUE_HPP
#define CszTASKQUEUE_HPP


namespace Csz
{
	//max heap
	#define TQComp [](const Data& T_lhs,const Data& T_rhs)\
	{\
		return T_lhs.priority< T_rhs.priority;\
	}

	template<class Parameter,int TASKNUM>
	TaskQueue<Parameter,TASKNUM>::TaskQueue():stop(false)
	{
		
	}

	template<class Parameter,int TASKNUM>
	TaskQueue<Parameter,TASKNUM>::~TaskQueue()
	{
		task_queue.clear();
	}

	template<class Parameter,int TASKNUM>
	inline void TaskQueue<Parameter,TASKNUM>::_Push(typename TaskQueue<Parameter,TASKNUM>::Type* T_task)
	{
		if (nullptr== T_task)
			return ;
		std::unique_lock<bthread::Mutex> guard(queue_mutex);
		while (!stop && (task_queue.size()== TASKNUM))
		{
			//sleep 3s
			push_cond.wait_for(guard,3000000);
		}
		if (stop)
		{
			return ;
		}
        Data data;
        data.priority= 0;
        data.func= std::move(T_task->first);
        data.parameter= T_task->second;
		task_queue.emplace_back(data);
		std::push_heap(task_queue.begin(),task_queue.end(),TQComp);
		guard.unlock();
		pop_cond.notify_one();
		return ;
	}

	template<class Parameter,int TASKNUM>
	inline bool TaskQueue<Parameter,TASKNUM>::_Pop(typename TaskQueue<Parameter,TASKNUM>::Type* T_task)
	{
		if (nullptr== T_task)
			return false;
		std::unique_lock<bthread::Mutex> guard(queue_mutex);
		while (!stop && Empty())
		{
			//sleep 3s
			pop_cond.wait_for(guard,3000000);
		}
		if (stop)
		{
			return false;
		}
		std::pop_heap(task_queue.begin(),task_queue.end(),TQComp);
		auto& ret= task_queue.back();
		T_task->first= std::move(ret->func);
		T_task->second= ret->parameter;
		task_queue.pop_back();
		guard.unlock();
		push_cond.notify_one();
		return true;
	}

	template<class Parameter,int TASKNUM>
	void TaskQueue<Parameter,TASKNUM>::Push(typename TaskQueue<Parameter,TASKNUM>::Type* T_task)
	{
		_Push(T_task);
		return ;
	}

	template<class Parameter,int TASKNUM>
	bool TaskQueue<Parameter,TASKNUM>::Pop(typename TaskQueue<Parameter,TASKNUM>::Type* T_task)
	{
		return _Pop(T_task);
	}

	template<class Parameter,int TASKNUM>
	void TaskQueue<Parameter,TASKNUM>::Stop()
	{
		{
			std::unique_lock<bthread::Mutex> guard(queue_mutex);
			stop= true;
		}
		push_cond.notify_all();
		pop_cond.notify_all();
		return ;
	}
}

#endif //CszTASKQUEUE_HPP
