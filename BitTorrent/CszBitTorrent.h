#ifndef CszBITTORRENT_H
#define CszBITTORRENT_H
////////////////////////
#define CszTest

#include "../Error/CszError.h"
#include "../Web/CszWeb.h" //CszHttpRequest,CszHttpResponse,CszUrlEscape
#include "../Sock/CszSocket.h" //CszTcpConnect,CszFcntl,CszSocketHostServNull
#include "sha1.h" 
#include "../CszNonCopyAble.hpp"
#include <unistd.h> //write in CszBStr
#include <sys/uio.h> //writev
#include <ctime> //time
#include <cstdint> //uint64_t
#include <cstdlib> //srand,rand
#include <sys/select.h>
#include <stdexcept> //invalid_argument,out_of_rangea
#include <string>
#include <vector>
#include <unordered_map>
#include <functional> //std::function,binary_function
#include <utility> //std::make_pair
#include <memory> //shared_ptr


namespace Csz
{
	class TorrentFile;
	class BDataType : public Csz::NonCopyAble
	{
		public:
			virtual ~BDataType(){};
			virtual void Decode(std::string&)= 0;
			virtual void ReadData(const std::string&,TorrentFile*)=0;
#ifdef CszTest
			virtual void COutInfo()= 0;
#endif
	};

	class BInt : public BDataType
	{
		public:
			BInt();
			~BInt();
			void Decode(std::string&);
			void ReadData(const std::string&,TorrentFile*);
#ifdef CszTest
			void COutInfo();
#endif
		private:
			std::uint64_t data;
	};

	class BStr : public BDataType
	{
		public:
			BStr();
			~BStr();
			void Decode(std::string&);
			void ReadData(const std::string&,TorrentFile*);
#ifdef CszTest
			void COutInfo();
#endif
		//in CszBDict::Decode
		//private:
			std::string data;
	};

	class BList : public BDataType
	{
		public:
			BList();
			~BList();
			void Decode(std::string&);
			void ReadData(const std::string&,TorrentFile*);
#ifdef CszTest
			void COutInfo();
#endif
		private:
			std::vector<BDataType*> data;
	};

	class BDict : public BDataType
	{
		public:
			BDict();
			~BDict();
			void Decode(std::string&);
			void ReadData(const std::string&,TorrentFile*);
			//BDataType* Search(std::string&) const;
#ifdef CszTest
			void COutInfo();
#endif
		private:
			std::unordered_map<std::string,BDataType*> data;
	};

	//d***e
	struct GetDictLength : public std::binary_function<const char*,int,int>
	{
		int operator()(const char* T_str,int T_len) const;
	};

//tracker
	struct TrackerInfo
	{
        //tcp set -1,udp set -2
		TrackerInfo():socket_fd(-3){}
		TrackerInfo(TrackerInfo&&);
		std::string host;
		std::string serv;
		std::string uri;
		int socket_fd;
	};

	class Tracker
	{
		public:
			Tracker();
			~Tracker();
			std::vector<int> RetSocket() const;
			void Connect();
			void SetTrackInfo(TrackerInfo);
			void SetInfoHash(std::string);
			void SetParameter(std::string);
			void GetPeerList(HttpRequest*,HttpResponse*,std::shared_ptr<CacheRegio>,int);
#ifdef CszTest
			void COutInfo();
#endif
		private:
			void Capturer(HttpResponse*,std::shared_ptr<CacheRegio>,const int&);
			void Delivery(HttpRequest*,const int&,const std::string&);
		private:
			std::vector<TrackerInfo> info;
			std::string info_hash;
			std::string parameter_msg;
	};

	class TorrentFile
	{
		private:
			struct File
			{
				File():length(0){}
				File(std::string T_file_path,uint64_t T_length):file_path(T_file_path),length(T_length){}
				std::uint64_t length;
				std::string file_path;
			};
			struct Info
			{
				Info():length(0),piece_length(0),single(true){}
				std::vector<File> files;
				std::string pieces;
				//单文件模式
				std::string name;
				std::uint64_t length;
				//
				std::uint64_t piece_length;
				bool single;
			};
		private:
			Info infos;
			//std::string announce;
			std::vector<std::string> announce_list;
			std::string comment;
			std::string create_by;
			std::uint64_t creation_date;
		public:
			//函数注册器
			std::unordered_map<std::string,std::function<void(void*)>> name_data;
			void GetTrackInfo(Tracker*);
			std::uint64_t GetTotal() const;
			std::string GetIndexHash(int);
			TorrentFile();
			~TorrentFile();
#ifdef CszTest
			void COutInfo();
#endif
	};

}
#endif
