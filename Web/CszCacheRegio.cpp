#include "CszWeb.h"

namespace Csz
{
	std::weak_ptr<CacheRegio> CacheRegio::singleton;
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
	std::shared_ptr<CacheRegio> CacheRegio::GetSingleton()
	{
		std::shared_ptr<CacheRegio> ret;
		//must atomic
		if (!(ret= singleton.lock()))
		{
			ret.reset(new CacheRegio(),[](const CacheRegio* T_cache_regio)
					{
						delete T_cache_regio;
					});
			singleton= ret;
#ifdef CszTest
			printf("not have shared space,so create share space\n");
#endif
		}
#ifdef CszTest
		printf("return singleton\n");
#endif
		return ret;
	}
	//设置超时
	inline int CacheRegio::AddCache(int T_socket_fd)
	{
		while (read_cnt<= 0)
		{
#ifdef CszTest
			printf("no data available\n");
#endif
			if ((read_cnt= read(T_socket_fd,read_buf,CACHESIZE))< 0)
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
	std::string CacheRegio::ReadLine(int T_socket_fd)
	{
		std::string line;
		line.reserve(32);
		for (int i= 0; i< CACHESIZE; ++i)
		{
			if ((read_cnt= AddCache(T_socket_fd))<= 0)
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
	//设置超时
	void CacheRegio::ReadBuf(const int& T_socket_fd,const int& T_save_fd,int T_save_num)
	{
		int curr_num;
		while (T_save_num> 0)
		{
			curr_num= T_save_num> read_cnt ? read_cnt : T_save_num;
			curr_num= write(T_save_fd,read_ptr,curr_num);
			T_save_num-= curr_num;
			read_cnt-= curr_num;
			if (read_cnt<= 0)
				AddCache(T_socket_fd);
		}
		return ;
	}
}