#include "CszWeb.h"
#include <iostream>
#define IOVECSIZE 32

int main(int argc,char** argv)
{
	if (argc< 3)
		return 0;
	int socket_fd= Csz::TcpConnect(argv[1],argv[2]);
	Csz::HttpRequest request;
	request.SetFirstLine("GET / HTTP/1.1");
	{
		struct sockaddr_storage addr;
		socklen_t addr_len=sizeof(addr);
		if (getpeername(socket_fd,(struct sockaddr*)&addr,&addr_len)< 0)
		{
			Csz::ErrSys("getpeername can't get socket addr info");
		}
		char* addr_str= Csz::SocketNtoPHost((struct sockaddr*)&addr,addr_len);
#ifdef CszTest
		printf("ip:%s\n",addr_str);
#endif
		if (NULL== addr_str)
		{
			Csz::ErrSys("host %s",argv[1]);
		}
		request.SetHeader("Host",addr_str);
	}
	request.SetHeader("Connection","Close");
	request.SetHeader("Cache-control","no-cache");
	request.SetHeader("User-Agent","Super Max");
	request.SetHeader("Accept","text/html");
//	request.SetHeader("if-Modified-Since","Thu, 09 Feb 2012 09:07:57 GMT");
	struct iovec data_arry[IOVECSIZE];
	size_t msg_num= request.size()>= IOVECSIZE? IOVECSIZE : request.size()+ 1;
	request.BindMsg(data_arry,msg_num);
#ifdef CszTest
	printf("send info: \n");
	request.COutInfo();
#endif
	writev(socket_fd,data_arry,msg_num);
	Csz::CacheRegio cache;
	Csz::HttpResponse response;
#ifdef CszTest
	printf("Catpurer info:\n");
#endif
	response.Capturer(socket_fd,&cache);
    std::cerr<<"body info:\n"<<response.GetBody()<<"\n"<<"body len="<<response.GetBody().size();
	close(socket_fd);
	//exit使shared_ptr失效(具体使引用计数失效)
	//exit(0); 
	return 0;
}
