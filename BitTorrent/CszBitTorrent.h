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
#include <sys/select.h> //select
#include <sys/uio.h> //writev
#include <ctime> //time
#include <cstdint> //uint64_t
#include <cstdlib> //srand,rand
#include <sys/select.h>
#include <stdexcept> //invalid_argument,out_of_rangea
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional> //std::function,binary_function
#include <utility> //std::make_pair
#include <memory> //shared_ptr
#include <cstring> //bzero,memcpy
#include <arpa/inet.h> //htonl
//brpc
#include <butil/memory/singleton_on_pthread_once.h> //brpc::singleton
//micro
#include "CszMicro.hpp"

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
			std::vector<std::string> GetPeerList(HttpRequest*,HttpResponse*,CacheRegio*const,int);
			const std::string& GetInfoHash()const {return info_hash;}
#ifdef CszTest
			void COutInfo();
#endif
		private:
			void Capturer(HttpResponse*,CacheRegio*const,const int&);
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
			std::uint64_t GetFileTotal() const;
			std::uint32_t GetIndexTotal() const;
			char GetEndBit() const;
			std::string GetIndexHash(int);
			TorrentFile();
			~TorrentFile();
#ifdef CszTest
			void COutInfo();
#endif
	};
    
    class PeerManager
    {
		private:
            //PeerManager(const std::vector<std::string> &T_socket_list);
			PeerManager()= default;
            ~PeerManager();
		public:
			static PeerManager* GetInstance()
			{
				return butil::get_leaky_singleton<PeerManager>();
			}
		private:
			friend void butil::GetLeakySingleton<PeerManager>::create_leaky_singleton();
        public:
            void LoadPeerList(const std::vector<std::string> &T_socket_list);
            void AddSocket(const int T_socket)
            {
                if (T_socket>= 0)
                    peer_list.emplace(T_socket);
            }
			const std::vector<int>& RetSocketList()const {return peer_list;}
			void CloseSocket(int T_socket);
			void CloseSocket(std::vector<int>* T_sockets);
        private:
            void _LoadPeerList(const std::string& T_socket_list);
			void _Connected(std::vector<int>& T_ret);
            void _Verification(std::vector<int>& T_ret);
			void _SendBitField(std::vector<int>& T_ret);
        private:
            std::set<int> peer_list;
    };

    //bt message type
	//base method
/*	struct PeerMessage
	{
	};
*/
    //1.handshake
    //TODO pstr is fixation
    class HandShake
    {
		private:
			HandShake();
			~HandShake()= default;
		public:
			static HandShake* GetInstance()
			{
				return butil::get_leaky_singleton<HandShake>();
			}
		private:
			friend void butil::GetLeakySingleton<HandShake>::create_leaky_singleton();
        private:
			//pstrlen pstr reserved infohash peerid
            char data[68];
        public:
            bool SetParameter(const char* T_reserved,const char* T_info_hash,const char* T_peer_id);
            const char* GetSendData()const {return data;}
			const char* operator()(){return GetSendData();}
			bool Varification(const char* T_str)const;
			//micro
            int GetDataSize()const {return 68;}
    };

    //2.keep alive
    class KeepAlive
    {
		private:
			char alive_pack[4];
		public:
			KeepAlive(){bzero(alive_pack,sizeof(alive_pack));}
			const char* GetSendData()const {return alive_pack;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize()const {return 4;}
    };

    //3.choke id= 0
    class Choke
    {	
		private:
			char choke[5];
		public:
			Choke()
			{
				bzero(choke,sizeof(choke));
				*reinterpret_cast<int32_t*>(choke)= htonl(1);
				//set id
				choke[4]= 0;
			}
			const char* GetSendData()const {return choke;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize()const {return 5;}
    };

    //4.unchoke id= 1
    class UnChoke
    {
		private:
			char unchoke[5];
		public:
			UnChoke()
			{
				bzero(unchoke,sizeof(unchoke));
				*reinterpret_cast<int32_t*>(unchoke)= htonl(1);
				//set id 
				unchoke[4]= 1;
			}
			const char* GetSendData()const {return unchoke;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize()const {return 5;}
    };
    
    //5.interested id= 2
    class Interested
    {
		private:
			char interested[5];
		public:
			Interested()
			{
				bzero(interested,sizeof(interested));
				*reinterpret_cast<int32_t*>(interested)= htonl(1);
				//set id
				interested[4]= 2;
			}
			const char* GetSendData()const {return interested;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize()const {return 5;}
    };

	//6.not interested id= 3
	class UnInterested
	{
		private:
			char uninterested[5];
		public:
			UnInterested()
			{
				bzero(uninterested,sizeof(uninterested));
				*reinterpret_cast<int32_t*>(uninterested)= htonl(1);
				//set id
				uninterested[4]= 3;
			}
			const char* GetSendData()const {return uninterested;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize()const {return 5;}
	};

	//7.have id= 4
	class Have
	{
		private:
			char have[9];
		public:
			Have()
			{
				bzero(have,sizeof(have));
				*reinterpret_cast<int32_t*>(have)= htonl(5);
				//set id
				have[4]= 4;
			}
			void SetParameter(int32_t T_index)
			{
				//TODO network byte
				*reinterpret_cast<int32_t*>(have+ 5)= htonl(T_index);
				return ;
			}
			const char* GetSendData()const {return have;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize()const {return 9;}
	};

	//8.bitfield id= 5
	class BitField
	{
		private:
			//valid bit length <= prefix_and_bit_field.size()* 8
			std::string prefix_and_bit_field;
		public:
			BitField()
			{
				//init prefix
				//thorw exception std::length_error
				prefix_and_bit_field.resize(5,0);
				//set id
				prefix_and_bit_field[4]= 5;
			}
			void SetParameter(std::string T_bit_field);
			void FillBitField(int32_t T_index);
			bool CheckPiece(int32_t T_index);
			const char* GetSendData()const{return prefix_and_bit_field.c_str();}
			const char* operator()(){return GetSendData();}
			int GetDataSize()const {return prefix_and_bit_field.size();}
			bool GameOver(const char T_end_bit)const;
			std::vector<int32_t> LackNeedPiece(const char* T_bit_field,const int T_len);
			std::vector<int32_t> LackNeedPiece(const std::string);
		private:
			void _SetParameter(std::string T_bit_field);
			void _SetPrefixLength();
			void _Clear();
	};

	//9.request id= 6 or cancle= id= 8
	class ReqCleBase
	{
		private:
			char data[17];
		public:
			ReqCleBase(char T_id)
			{
				bzero(data,sizeof(data));
				//set id
				data[4]= T_id;
			}
			void SetParameter(int32_t T_index,int32_t T_begin,int32_t T_length);
			const char* GetSendData()const {return data;}
			const char* operator()(){return GetSendData();}
	};
 
	//9.1 Request id= 6
	class Request
	{
		private:
			//impl
			ReqCleBase request;
		public:
			//set id
			Request():request(6){}
			void SetParameter(int32_t T_index,int32_t T_begin,int32_t T_length){request.SetParameter(T_index,T_begin,T_length);}
			const char* GetSendData()const {return request.GetSendData();}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize(){return 17;}
	};

	//10.piece id =7
	using PieceSliceType= std::string;
	class Piece
	{
		private:
			std::string prefix_and_slice;
		public:
			Piece()
			{
				//init prefix
				prefix_and_slice.resize(13,0);
				//set id
				prefix_and_slice[4]= 7;
			}
			void SetParameter(int32_t T_index,int32_t T_begin,PieceSliceType T_slice);
			const char* GetSendData()const {return prefix_and_slice.c_str();}
			const char* operator()(){return GetSendData();}
			int GetDataSize(){return prefix_and_slice.size();}
		private:
			void _SetPrefixLength();
	};	

	//9.2 Cancle id= 8
	class Cancle
	{
		private:
			//impl
			ReqCleBase cancle;
		public:
			//set id
			Cancle():cancle(8){}
			void SetParameter(int32_t T_index,int32_t T_begin,int32_t T_length){cancle.SetParameter(T_index,T_begin,T_length);}
			const char* GetSendData()const {return cancle.GetSendData();}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize(){return 17;}
	};

	//11.port id= 9
	class Port
	{
		private:
			char port[7];
		public:
			Port()
			{
				bzero(port,sizeof(port));
				//set id
				port[4]= 9;
			}
			void SetParameter(int32_t T_port)
			{
				*reinterpret_cast<int32_t*>(port+ 5)= htonl(T_port);
				return ;
			}
			const char* GetSendData()const {return port;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize(){return 7;}
	};
	
	//set singleton
	//local bitfield 
	class LocalBitField : public BitField
	{
		//TODO valid bit length
		private:
			LocalBitField()= default;
			~LocalBitField()= default;
		public:
			static LocalBitField* GetInstance()
			{
				return butil::get_leaky_singleton<LocalBitField>();
			}
		private:
			friend void butil::GetLeakySingleton<LocalBitField>::create_leaky_singleton();
		private:
			char end_bit;
		public:
			void SetEndBit(const char T_ch){end_bit= T_ch;}
			bool GameOver()const{return BitField::GameOver(end_bit);}
			void RecvHave(int T_socket,int32_t T_index);
			void RecvBitField(int T_socket,const char* T_bit_field,const int T_eln);
	};

	//DownSpeed
	class DownSpeed
	{
		private:
			DownSpeed()= default;
			~DownSpeed()= default;
		public:
			static DownSpeed* GetInstance()
			{
				return butil::get_leaky_singleton<DownSpeed>();
			}
		private:
			friend void butil::GetLeakySingleton<DownSpeed>::create_leaky_singleton();
		private:
			//10s update
			//priority queue
			std::vector<std::pair<int,uint32_t>> queue;
		public:
			void AddMember(const int T_socket)
			{
				queue.emplace_back(std::make_pair(T_socket,0));
			}
			void AddTotal(const int T_socket,const uint32_t T_speed);
			void AddTotal(std::vector<std::pair<int,uint32_t>>& T_queue);
			//four
			std::vector<int> RetSocket();
	};

	//Need Piece
	class NeedPiece
	{
		private:
			NeedPiece():cur_id(0){};
			~NeedPiece()= default;
		public:
			static NeedPiece* GetInstance()
			{
				return butil::get_leaky_singleton<NeedPiece>();
			}
		private:
			friend void butil::GetLeakySingleton<NeedPiece>::create_leaky_singleton();
		public:
			/*using NPComp= [](const std::pair<uint32_t,uint8_t>& T_lhs,const std::pair<uint32_t,uint8_t>& T_rhs)
			{
				return T_lhs.second.size()> T_rhs.second.size();
			};
			*/
		private:
			//piece map N socket
			//min priority
			//index->counter
			std::vector<std::pair<int32_t,std::shared_ptr<std::vector<int>>>> queue;
			//socket may be reuse
			//id->choke/unchoke and interested/uninterested 
			std::unordered_map<int,uint8_t> socket_queue;
			//socket->id
			std::unordered_map<int,int> socket_cur;
			int cur_id;
		public:
			void PushNeed(const std::vector<int32_t>* T_index,const int T_socket);
			void PushNeed(const int32_t T_index,int T_socket);
			std::pair<int32_t,std::vector<int>> PopNeed();
			bool Empty()const;
			void SocketMapId(std::vector<int>& T_ret);
			void NPChoke(int T_socket);
			void NPUnChoke(int T_socket);
			void NPInterested(int T_socket);
			void NPUnInterested(int T_socket);
		private:
			bool _SetSocketStatus(int T_socket,char T_status);
			bool _ClearSocketStatus(int T_socket,char T_status);
	};

	//select & switch message type
	struct SelectSwitch
	{
		static fd_set rset_save;
		struct Parameter
		{
			Parameter():socket(-1),len(0),buf(nullptr),cur_len(){}
			~Parameter(){if (buf!= nullptr) delete[] buf;}
			int socket;
			//not include id len
			int32_t len;
			char* buf;
			int cur_len;
		};
		bool operator()()const;
		static void DKeepAlive(Parameter* T_data);
		static void DChoke(Parameter* T_data);
		static void DUnChoke(Parameter* T_data);
		static void DInterested(Parameter* T_data);
		static void DUnInterested(Pararmeter* T_data);
		static void DHave(Parameter* T_data);
		static void DBitField(Parameter* T_data);
		static void AsyncDBitField(Parameter* T_data);
		static void DRequest(Parameter* T_data);
		static void DPiece(Pararmeter* T_data);
		static void AsyncDPiece(Parameter* T_data);
		static void DCancle(Parameter* T_data);
		static void DPort(Parameter* T_data);
	};
}
#endif
