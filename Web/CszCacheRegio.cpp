#include "CszWeb.h"
#include <sstream> //ostringstream
#include <cstdlib> //getenv
#include <unistd.h> //read
#include <string> //stoul
#include <cstring> //strerror
#define CACHESIZE (1024* 1)

#ifdef CszTest
#include <cstdio>
#endif

namespace Csz
{
	CacheRegio::CacheRegio():read_cnt(0)
	{
		size_t len= CACHESIZE;
		try
		{
			char* cache_size= getenv("CszWebCache");
			if (NULL== cache_size)
			{
				len= CACHESIZE;
			}
			else
			{
				try
				{
#ifdef CszTest
					printf("CacheRegio get env %s\n",cache_size);
#endif
					len= std::stoul(cache_size);
				}
				catch (...)
				{
#ifdef CszTest
					printf("CacheRegio can't env change integer\n");
#endif
					len= CACHESIZE;
				}
			}
			read_buf= new char[len];
			read_ptr= read_buf;
		}
		catch(const std::bad_alloc& T_e)
		{
			read_ptr= read_buf= nullptr;
			Csz::ErrQuit(T_e.what());
			//throw;
		}
#ifdef CszTest
		printf("read_ptr:%p,size:%d,read_cnt:%d\n",read_ptr,CACHESIZE,read_cnt);
#endif
		/*
		//return nullptr;
		read_buf= new(std::nothrow) char[CACHESIZE]{0};
		read_ptr= read_buf;
		*/
	}

	CacheRegio::~CacheRegio()
	{
#ifdef CszTest
		read_buf== nullptr? printf("destruct read_buf is nullptr\n") : printf("destruct read_buf have space\n");
#endif
		if (read_buf!= nullptr)
		{
			delete[] read_buf;
			read_buf= nullptr;
		}
	}

	//TODO 设置超时
	inline int CacheRegio::AddCache(int T_socket)
	{
		while (read_cnt<= 0)
		{
#ifdef CszTest
			printf("no data available\n");
#endif
			if ((read_cnt= read(T_socket,read_buf,CACHESIZE))< 0)
			{
				if (EINTR== errno)
					continue;
				return -1;
			}
			else if (0== read_cnt)
			{
				return 0;
			}
			else //> 0
			{
				//init read_ptr
				read_ptr= read_buf;
				break;
			}
		}
#ifdef CszTest
	//	printf("have data available:%d\n",read_cnt);
#endif
		return read_cnt;
	}

	std::string CacheRegio::ReadLine(int T_socket)
	{
		std::string line;
		line.reserve(32);
		for (int i= 0; i< CACHESIZE; ++i)
		{
			if ((read_cnt= AddCache(T_socket))<= 0)
			{
				if (0== read_cnt)
				{
					Csz::ErrMsg("Readline can't read',peer socket close,get EOF");
				}
				else
				{
					Csz::ErrMsg("ReadLine can't read,%s",strerror(errno));
				}
				return line;
			}
			else if('\n'== *read_ptr)
			{
				++read_ptr;
				--read_cnt;
				break;
			}
			line.append(1,*read_ptr);
			++read_ptr;
			--read_cnt;
		}
		return line;
	}

	//TODO 设置超时
	std::string CacheRegio::ReadBuf(const int T_socket,int T_save_num)
	{   
        std::string ret_str;
		int curr_num;
		while (T_save_num> 0)
		{
			curr_num= T_save_num> read_cnt ? read_cnt : T_save_num;
            ret_str.append(read_ptr,curr_num);
			T_save_num-= curr_num;
			read_cnt-= curr_num;
			if (read_cnt<= 0)
				AddCache(T_socket);
		}
		return std::move(ret_str);
	}
}

#undef CACHESIZE
