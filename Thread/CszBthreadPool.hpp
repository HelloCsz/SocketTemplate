#ifndef CszBTHREADPOOL_HPP
#define CszBTHREADPOOL_HPP

namespace Csz
{
	template<class Parameter,int TASKNUM>
	void BthreadPool<Parameter,TASKNUM>::Init(int T_num)
	{
		pool.resize(T_num);
		for (auto& val : pool)
		{
			//normal func is two parameter
			if (bthread_start_background(&val,NULL,&Run,(void*)this)!= 0)
			{
				Csz::ErrQuit("Bthrad Pool create bthread failed");
			}
		}
	}

	template<class Parameter,int TASKNUM>
	void BthreadPool<Parameter,TASKNUM>::Push(typename TaskQueue<Parameter,TASKNUM>::Type* T_task)
	{
		task_list.Push(T_task);
		return ;
	}


	template<class Parameter,int TASKNUM>
	bool BthreadPool<Parameter,TASKNUM>::Pop(typename TaskQueue<Parameter,TASKNUM>::Type* T_task)
	{
		return task_list.Pop(T_task);
	}


	template<class Parameter,int TASKNUM>
	void* BthreadPool<Parameter,TASKNUM>::Run(void* T_this)
	{       
		auto cur_this= static_cast<BthreadPool*>(T_this);
		typename TaskQueue<Parameter,TASKNUM>::Type task;
		while (cur_this->Pop(&task))
		{
			task.first(task.second);
		}
		return nullptr;
	}

	template<class Parameter,int TASKNUM>
	void BthreadPool<Parameter,TASKNUM>::Stop()
	{
		Csz::LI("[%s->%d]bthread join",__func__,__LINE__);
		task_list.Stop();
		for (const auto& val : pool)
		{
			bthread_join(val,nullptr);
		}
		if (!pool.empty())
		{
			pool.clear();
		}
        return ;
	}

	template<class Parameter,int TASKNUM>
	BthreadPool<Parameter,TASKNUM>::~BthreadPool()
	{
		Stop();
	}
}

#endif //CszBTHREADPOOL_HPP

