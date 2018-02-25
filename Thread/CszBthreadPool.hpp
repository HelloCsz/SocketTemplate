#ifndef CszBTHREADPOOL_HPP
#define CszBTHREADPOOL_HPP



template<class Parameter,int TASKNUM>
void BthreadPool<Parameter,TASKNUM>::BthreadPool(int T_num)
{
	pool.resize(T_num);
	for (auto& val : pool)
	{
		//normal func is two parameter
		if (bthread_start_background(&val,NULL,&Run,this)!= 0)
		{
			Csz::ErrQuit("Bthrad Pool create bthread failed");
		}
	}
}

template<class Parameter,int TASKNUM>
void BthreadPool<Parameter,TASKNUM>::Push(TaskQueue<Parameter,TASKNUM>::Type* T_task)
{
	task_list(T_task);
	return ;
}


template<class Parameter,int TASKNUM>
bool BthreadPool<Parameter,TASKNUM>::Pop(TaskQueue<Parameter,TASKNUM>::Type* T_task)
{
	return task_list(T_task);
}


template<class Parameter,int TASKNUM>
static void* BthreadPool<Parameter,TASKNUM>::Run(void* T_this)
{
	auto cur_this= static_cast<BthreadPool*>(T_this);
	TaskQueue<Paramter,TASKNUM> task;
	while (cur_this->Pop(&task))
	{
		task.first(task.second);
	}
	return nullptr;
}

template<class Parameter,int TASKNUM>
BthreadPool<Parameter,TASKNUM>::Stop()
{
	task_list.Stop();
	for (const auto& val : pool)
	{
		bthread_join(val);
	}
}
#endif //CszBTHREADPOOL_HPP
