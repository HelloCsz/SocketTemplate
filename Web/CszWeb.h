#ifndef CszWEB_H
#define CszWEB_H
#include "../CszNonCopyAble.hpp"
#include "../Error/CszError.h"
#include <sys/types.h> //msghdr
#include <string> //stoi,stoul
#include <vector>
#include <unordered_map> //unordered_map
//#include <sstream> //istringstream

#define CszTest
namespace Csz
{
	struct UrlEscape : public std::unary_function<std::string,std::string>
	{
		std::string operator()(std::string);
	};

	class HttpRequest
	{
		public:
			HttpRequest();
			void SetFirstLine(std::string);
			void SetFirstLine(const char*);
			void SetHeader(std::string,std::string);
			void SetHeader(const char*,const char*);
			//empty
			void SetBody();
			//iovec数组多1,存放/r/n/r/n
			bool BindMsg(struct iovec*,const size_t) const;
			size_t size()const;
		public:
#ifdef CszTest
			void COutInfo()const;
#endif
		private:
			std::string head_end;
			std::vector<std::string> data;
	};

	class CacheRegio;
	class HttpResponse
	{
		public:
			bool Capturer(const int T_socket,CacheRegio* T_cache);
			void Clear();
			int GetStatus()const;
			const std::string SearchHeader(const std::string* T_header_name);
            std::string GetBody()const{return body;}
			void COutInfo()const;
		private:
			std::string status_line;
			std::string body;
			std::unordered_map<std::string,std::string> header_data;
		private:
			void _CatchStatusLine(std::string&&);
			void _CatchHeader(std::string&&);
			void _SaveBody(const int T_socket,CacheRegio* T_cache);
	};

	//目的:并非多线程共享一个内存
	class CacheRegio : public NonCopyAble
	{
		private:
			int read_cnt;
			char* read_ptr;
			char* read_buf;
		private:
			int AddCache(int T_socket);
		public:
            CacheRegio();
			~CacheRegio();
			std::string ReadLine(int T_socket);
			std::string ReadBuf(const int T_socket,int T_save_num);
			//empty
			void Clear();
	};
}
#endif
