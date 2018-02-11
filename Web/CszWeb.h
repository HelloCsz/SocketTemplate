#ifndef CszWEB_H
#define CszWEB_H
#include "../Sock/CszSocket.h" //TcpConnect,SocketNtoPHost
#include "../Signal/CszSignal.h" //Signal
#include "../CszNonCopyAble.hpp"

#include <sys/types.h> //msghdr
#include <sys/socket.h> //getsockname
#include <sys/uio.h> //writev
#include <netinet/in.h> //sockaddr_storage
#include <unistd.h> //read,getpid,close,write
#include <cstring> //strerror
#include <cstdio> //fopen,fileno,fclose,snprintf
#include <cstdlib> //getenv
#include <cctype> //isalnum
#include <ctime> //time
#include <string> //stoi,stoul
#include <vector>
#include <unordered_map> //unordered_map
//#include <sstream> //istringstream
#include <memory> //shared_ptr


#define CACHESIZE 1024* 1
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
			void ClearMethod();
		public:
#ifdef CszTest
			void COutInfo()const;
#endif
		private:
			const std::string head_end;
			std::vector<std::string> data;
	};

	class CacheRegio;
	class HttpResponse
	{
		public:
			void Capturer(const int&,CacheRegio*const);
			void Clear();
			int GetStatus()const;
			const std::string SearchHeader(const std::string&);
            std::string GetBody()const{return body;}
#ifdef CszTest
			void COutInfo()const;
#endif
		private:
			std::string status_line;
			std::string body;
			std::unordered_map<std::string,std::string> header_data;
		private:
			void CatchFirstLine(std::string&&);
			void CatchHeader(std::string&&);
			void SaveBody(const int&,CacheRegio*const);
	};

	//目的:并非多线程共享一个内存
	class CacheRegio : public NonCopyAble
	{
		private:
			int read_cnt;
			char* read_ptr;
			char* read_buf;
		private:
			int AddCache(int);
		public:
            CacheRegio();
			~CacheRegio();
			std::string ReadLine(int);
			std::string ReadBuf(const int&,int);
			//empty
			void Clear();
	};
}
#endif
