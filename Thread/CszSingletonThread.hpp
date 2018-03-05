#ifndef CszSINGLETONTHREAD_HPP
#define CszSINGLETONTHREAD_HPP
#include "CszThread.hpp"
#include <butil/memory/singleton_on_pthread_once.h> //butil::singleton

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
            void Push(typename TaskQueue<Parameter,TASKNUM>::Type* T_task){pool.Push(T_task);}
            void Pop(typename TaskQueue<Parameter,TASKNUM>::Type* T_task){pool.Pop(T_task);}
    };
}

#endif //CszSINGLETONTHREAD_HPP
