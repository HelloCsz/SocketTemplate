#ifndef Csz_SINGLETONTHREAD_HPP
#define Csz_SINGLETONTHREAD_HPP

namespace Csz
{
    template<class Parameter,int TASKNUM>
    class SingletonThread
    {
        private:
            SingletonThread()= default;
            ~SingletonThread()= default;
        public:
            static SingletonThread* GetInstance()
            {
                return butil::get_leaky_singleton<SingletonThread>();
            }
        private:
            friend void butil::GetLeakySingleton<SingletonThread>::create_leaky_singleton();
        private:
            BthreadPool<Parameter,TASKNUM> pool;
        public:
            void Init(int T_num){pool.Init(T_num);}
            void Push(TaskQueue::Type* T_task){pool.Push(T_task);}
            void Pop(TaskQueue::Type* T_task){pool.Pop(T_task);}
    }
}
