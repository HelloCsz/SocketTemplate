#include <string> //stoi
#include <cctype> //isdigit
#include "CszWeb.h"

#ifdef CszTest
#include <cstdio>
#endif 

namespace Csz
{
	bool HttpResponse::Capturer(const int T_socket,CacheRegio* T_cache)
	{
		if (nullptr== T_cache)
		{
			Csz::ErrMsg("Http Response can't Catpurer,CacheRegio is empty");
			return false;
		}
        //1.read status head
		std::string line= T_cache->ReadLine(T_socket);
		if (line.empty())
		{
			Csz::ErrMsg("Http Response failed,status is empty ");
			return false;
		}
		_CatchStatusLine(std::move(line));
        //2.check status
        if (GetStatus()!= 200)
        {
			Csz::ErrMsg("Http Response failed,status != 200");
			return false;
        }
		while (!(line= T_cache->ReadLine(T_socket)).empty())
		{
			if (4== line.size() && "\r\n\r\n"==line)
			{
				//catch body
				_SaveBody(T_socket,T_cache);
				break;
			}
			_CatchHeader(std::move(line));
		}
#ifdef CszTest
		printf("save header data info:\n");
		for (const auto& val : header_data)
		{
			printf("%s: %s\n",val.first.c_str(),val.second.c_str());
		}
#endif
		return true;
	}

	inline void HttpResponse::_CatchStatusLine(std::string&& T_line)
	{
		status_line=std::move(T_line);
#ifdef CszTest
		printf("status line:\n%s\n",status_line.c_str());
#endif
        return ;
	}

	inline void HttpResponse::_CatchHeader(std::string&& T_data)
	{
		/*
		//性能优化点
		std::istringstream string_io(T_data);
		std::string key,val;
		std::getline(string_io,key,':');
		//去掉空格
		string_io.get();
		std::getline(string_io,val,'\r');
		*/
		auto flag= T_data.find_first_of(':');
		std::string key(T_data,0,flag);
		//去掉空格
		flag+= 2;
		std::string val(T_data,flag,T_data.size()- flag);
		auto check= header_data.emplace(std::move(key),std::move(val));
		if (!check.second)
		{
			Csz::ErrMsg("Catpurer duplicate message:%s",T_data.c_str());
		}
#ifdef CszTest
		printf("%s\n",T_data.c_str());
#endif
        return ;
	}

	void HttpResponse::_SaveBody(const int T_socket,CacheRegio* T_cache) 
	{
		//TODO 大小写敏感
		auto result= header_data.find("Content-Type");
		if (header_data.end()== result)
		{
			return ;
		}
		int num;
		result= header_data.find("Content-Length");
		if (result!= header_data.end())
		{
            //throw exception
			num= std::stoi(result->second);
		}
		else
		{
			Csz::ErrMsg("SaveBody can't find Content-Length");
			return ;
		}
		body.assign(std::move(T_cache->ReadBuf(T_socket,num)));
        return ;
	}

	void HttpResponse::Clear()
	{
		//清除数据,但是不改变已申请的空间大小
		status_line.clear();
		header_data.clear();
        body.clear();
        return ;
	}

	int HttpResponse::GetStatus()const
	{
		int status= 0;
		for (int i= 0,len= status_line.size(); i< len; ++i)
		{
			if (' '== status_line[i])
			{
				while (++i< len)
				{
					if (isdigit(status_line[i]))
					{
						status= status* 10+ status_line[i]- '0';
					}
					else
						return status;
				}
			}
		}
        return -1;
	}

	//不能返回引用,因为在找不到情况下需要返回一个空数据,但空数据是临时变量
	//返回值需要const,因为引用可以修改到内部real数据,且key value不能修改,若
	//修改则破坏了容器的内部结构
	const std::string HttpResponse::SearchHeader(const std::string* T_header_name)
	{
		auto result= header_data.find(*T_header_name);
		if (header_data.end()== result)
			return "";
		return result->second;
	}

#ifdef CszTest
	void HttpResponse::COutInfo()const
	{
		printf("status line:%s\n",status_line.c_str());
		for (const auto& val : header_data)
		{
			printf("%s:%s\n",val.first.c_str(),val.second.c_str());
		}
	}
#endif
}
