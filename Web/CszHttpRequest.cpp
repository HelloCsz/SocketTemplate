#include "CszWeb.h"

namespace Csz
{
	HttpRequest::HttpRequest():head_end("\r\n\r\n")
	{
		data.push_back("");
	}
	void HttpRequest::SetFirstLine(std::string T_request_head)
	{
		if (data.empty())
		{
			Csz::ErrQuit("HttpRequest data is empty");
		}
		data[0].assign(std::move(T_request_head.append("\r\n")));
		//data.push_back(T_request_head.append("\r\n"));
	}
	void HttpRequest::SetFirstLine(const char* T_request_head)
	{
		SetFirstLine(std::string(T_request_head));
	}
	void HttpRequest::SetHeader(std::string T_name,std::string T_val)
	{
		data.push_back(std::move(T_name.append(": "+T_val+"\r\n")));
	}
	void HttpRequest::SetHeader(const char* T_name,const char* T_val)
	{
		SetHeader(std::string(T_name),std::string(T_val));
	}
	//iovec数组多1,存放/r/n/r/n
	bool HttpRequest::BindMsg(struct iovec* T_iovec,const size_t T_iovec_len) const
	{
		size_t iovec_index;
		size_t data_num= data.size();
		for (iovec_index= 0;iovec_index< T_iovec_len && iovec_index< data_num ;++iovec_index)
		{
			T_iovec[iovec_index].iov_base= (void*)data[iovec_index].c_str();
			T_iovec[iovec_index].iov_len= data[iovec_index].size();
		}
		if (iovec_index< T_iovec_len)
		{
			T_iovec[iovec_index].iov_base= (void*)head_end.c_str();
			T_iovec[iovec_index].iov_len= head_end.size();
		}
		else
		{
			Csz::ErrMsg("BindMsg RequestHead too long");
			return false;
		}
		return true;
	}
	size_t HttpRequest::size()const
	{
		return data.size();
	}
	void HttpRequest::ClearMethod()
	{
		//data.clear();
		//data.push_back("");
		data[0].assign("");
	}
#ifdef CszTest
	void HttpRequest::COutInfo()const
	{
		size_t num= 0;
		for (const auto& val : data)
		{
			printf("%s",val.c_str());
			num+= val.size();
		}
		num+= head_end.size();
		printf("total msg:%lu\n",num);
	}
#endif
}
