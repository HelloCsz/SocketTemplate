#ifndef CszBITTORRENT_H
#define CszBITTORRENT_H
////////////////////////
#define CszTest

#include "../Error/CszError.h"
#include "../Web/CszWeb.h" //CszHttpRequest,CszHttpResponse,CszUrlEscape
#include "../CszNonCopyAble.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <functional> //std::function,binary_function
#include <utility> //std::make_pair
#include <memory> //shared_ptr
#include <arpa/inet.h> //htonl
#include <pthread.h> //pthread_rwlock_t
//brpc
//#include <bthread/bthread.h>
//#include <bthread/condition_variable.h>
#include <butil/memory/singleton_on_pthread_once.h> //butil::singleton
#include <butil/resource_pool.h> //butil::ResourcePool
#include <bthread/mutex.h> //Mutex

#include "CszMicro.hpp"

#ifdef CszTest
#include <cstdio>
#include <bitset>
#endif

namespace Csz
{
	class TorrentFile;
	class BDataType : public Csz::NonCopyAble
	{
		public:
			virtual ~BDataType(){};
			virtual void Decode(std::string&)= 0;
			virtual void ReadData(const std::string&,TorrentFile*)=0;
			virtual void COutInfo()= 0;
	};

	class BInt : public BDataType
	{
		public:
			BInt();
			~BInt();
			void Decode(std::string&);
			void ReadData(const std::string&,TorrentFile*);
			void COutInfo();
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
			void COutInfo();
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
			void COutInfo();
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
			void COutInfo();
		private:
			std::unordered_map<std::string,BDataType*> data;
	};

	//d***e
	struct GetDictLength : public std::binary_function<const char*,int,int>
	{
		int operator()(const char* T_str,int T_len) const;
	};

    class Tracker;

	class TorrentFile
	{
        private:
            TorrentFile();
            ~TorrentFile();
        public:
            static TorrentFile* GetInstance()
            {
                return butil::get_leaky_singleton<TorrentFile>();
            }
        private:
            friend void butil::GetLeakySingleton<TorrentFile>::create_leaky_singleton();
		private:
			struct File
			{
				File():length(0){}
				File(std::string T_file_path,uint64_t T_length):file_path(std::move(T_file_path)),length(T_length){}
				std::uint64_t length;
				std::string file_path;
			};
			struct Info
			{
				Info():length(0),piece_length(0),single(true){}
				std::vector<File> files;
				//hash info
				std::string pieces;
				//单文件模式
				std::string name;
				std::uint64_t length;
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
			std::uint32_t GetIndexBitTotal() const;
			//char GetEndBit() const;
			std::string GetHash(int32_t T_index) const;
			//begin|length
			using WRITEINFO= std::pair<int32_t,int32_t>;
			//file name | begin | length
			using FILEINFO= std::pair<std::string,WRITEINFO>;
			std::vector<FILEINFO> GetFileName(int32_t T_index,int32_t T_begin,int32_t T_length) const;
			std::vector<FILEINFO> GetFileName(int32_t T_index) const;
            int32_t GetPieceLength(int32_t T_index) const;
            int32_t GetPieceBit(int32_t T_index) const;
            int32_t GetIndexEnd()const;
            int32_t GetIndexEndLength()const;
            int32_t GetIndexNormalLength()const;
            std::pair<bool,int32_t> CheckEndSlice(int32_t T_index) const;
            std::pair<bool,int32_t> CheckEndSlice(int32_t T_index,int32_t T_begin) const;
			void COutInfo();
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
            //torrent file call
			void SetTrackInfo(TrackerInfo);
			void SetInfoHash(std::string);
			std::vector<std::string> GetPeerList(int T_timeout);
			//const std::string& GetInfoHash()const {return info_hash;}
			std::string& GetAmId(){return am_id;}
			void COutInfo();
		private:
			void SetParameter(std::string);
			void _Capturer(const int T_socket);
			void _Delivery(const int T_socket,const std::string& T_host,const std::string& T_serv,const std::string& T_uri);
            //update http parameter msg
			void _UpdateReq();
            void _InitReq();
		private:
			std::string info_hash;
			std::string parameter_msg;
            std::string am_id;
			std::vector<TrackerInfo> info;
			HttpRequest request;
			HttpResponse response;
			CacheRegio cache;
	};
    
    //Peer Status
    struct PeerStatus
    {
        PeerStatus():am_choke(1),am_interested(0),peer_choke(1),peer_interested(0),unused(0){}
        unsigned char am_choke:1;
        unsigned char am_interested:1;
        unsigned char peer_choke:1;
        unsigned char peer_interested:1;
        unsigned char unused:4;
    };

    class PeerManager
    {
		private:
            //PeerManager(const std::vector<std::string> &T_socket_list);
			PeerManager():cur_id(0),had_file(false)
            {
#ifdef CszTest
                Csz::LI("construct Peer Manager");
#endif
                pthread_rwlock_init(&lock,NULL);
            }
            ~PeerManager();
		public:
			static PeerManager* GetInstance()
			{
				return butil::get_leaky_singleton<PeerManager>();
			}
		private:
			friend void butil::GetLeakySingleton<PeerManager>::create_leaky_singleton();
        public:
            //init peer socket
            void LoadPeerList(const std::vector<std::string> &T_socket_list);
            void AddSocket(const int T_socket);
			std::vector<int> RetSocketList();
			void CloseSocket(int T_socket);
			void CloseSocket(std::vector<int>* T_sockets);
            void SendHave(int32_t T_index);
            std::shared_ptr<bthread::Mutex> GetSocketMutex(int T_socket);
			int GetSocketId(int T_socket);
			void AmChoke(int T_socket);
			void AmUnChoke(int T_socket);
			void AmInterested(int T_socket);
			void AmUnInterested(int T_socket);
			void PrChoke(int T_socket);
			void PrUnChoke(int T_socket);
			void PrInterested(int T_socket);
			void PrUnInterested(int T_socket);
			void Optimistic();
            void SetHadFile(){had_file= true;}
			void COutInfo()const;
        private:
            void _LoadPeerList(const std::string& T_socket_list);
			void _Connected(std::vector<int>& T_ret);
            void _Verification(std::vector<int>& T_ret);
			void _SendBitField(std::vector<int>& T_ret);
        private:
            bool had_file;
            struct DataType
            {
				PeerStatus status;
                int id;
                std::shared_ptr<bthread::Mutex> mutex;
            };
            //socket have singleton id
            std::unordered_map<int,std::shared_ptr<PeerManager::DataType>> peer_list;
            int32_t cur_id;
            pthread_rwlock_t lock;
    };

    //bt message type
	//base method
/*	
 	struct PeerMessage
	{
	};
*/
    //1.handshake
    //TODO pstr is fixation
    class HandShake
    {
		private:
			HandShake();
			~HandShake()
            {
#ifdef CszTest
                Csz::LI("destructor Hand Shake");
#endif
            }
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
            void COutInfo();
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
#ifdef CszTest
                Csz::LI("constructor Choke");
#endif
				bzero(choke,sizeof(choke));
				*reinterpret_cast<int32_t*>(choke)= htonl(1);
				//set id
				choke[4]= 0;
			}
            ~Choke()
            {
#ifdef CszTest
                Csz::LI("destructor Choke");
#endif
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
#ifdef CszTest
                Csz::LI("constructor UnChoke");
#endif
				bzero(unchoke,sizeof(unchoke));
				*reinterpret_cast<int32_t*>(unchoke)= htonl(1);
				//set id 
				unchoke[4]= 1;
			}
            ~UnChoke()
            {
#ifdef CszTest
                Csz::LI("destructor UnChoke");
#endif
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
#ifdef CszTest
                Csz::LI("constructor Interested");
#endif
				bzero(interested,sizeof(interested));
				*reinterpret_cast<int32_t*>(interested)= htonl(1);
				//set id
				interested[4]= 2;
			}
            ~Interested()
            {
#ifdef CszTest
                Csz::LI("destructor Interested");
#endif
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
#ifdef CszTest
                Csz::LI("constructor UnInterested");
#endif
				bzero(uninterested,sizeof(uninterested));
				*reinterpret_cast<int32_t*>(uninterested)= htonl(1);
				//set id
				uninterested[4]= 3;
			}
            ~UnInterested()
            {
#ifdef CszTest
                Csz::LI("destructor UnInterested");
#endif
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
#ifdef CszTest
                Csz::LI("constructor Have");
#endif
				bzero(have,sizeof(have));
				*reinterpret_cast<int32_t*>(have)= htonl(5);
				//set id
				have[4]= 4;
			}
            ~Have()
            {
#ifdef CszTest
                Csz::LI("destructor Have");
#endif
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
            void COutInfo()
            {
                std::string out_info;
                out_info.reserve(32);
                char* p= have;
                out_info.append("[Have INFO]:len=");
                out_info.append(std::to_string(ntohl(*reinterpret_cast<int32_t*>(p))));
                out_info.append(";id="+ std::to_string(int(*(p+4))));
                out_info.append(";index="+std::to_string(ntohl(*reinterpret_cast<int32_t*>(p+ 5))));
				if (!out_info.empty())
					Csz::LI("%s",out_info.c_str());
            }   
	};

	//8.bitfield id= 5
	class BitField
	{
		private:
			//valid bit length <= prefix_and_bit_field.size()* 8
			std::string prefix_and_bit_field;
			int32_t total;
			int32_t cur_sum;
		public:
			BitField()
			{
#ifdef CszTest
                Csz::LI("constructor Bit Field");
#endif
				//init prefix
				//thorw exception std::length_error
				prefix_and_bit_field.resize(5,0);
				//set id
				prefix_and_bit_field[4]= 5;
				total= 0;
				cur_sum= 0;
			}
            ~BitField()
            {
#ifdef CszTest
                Csz::LI("destructor Bit Field");
#endif
            }
			//total bit len
			void SetParameter(std::string T_bit_field,int32_t T_total);
			void FillBitField(int32_t T_index);
			bool CheckPiece(int32_t& T_index);
			const char* GetSendData()const{return prefix_and_bit_field.c_str();}
			const char* operator()(){return GetSendData();}
			int GetDataSize()const {return prefix_and_bit_field.size();}
			bool GameOver()const;
			std::vector<int32_t> LackNeedPiece(const char* T_bit_field,const int T_len);
			std::vector<int32_t> LackNeedPiece(const std::string);
			void ProgressBar();
			void COutInfo() const;
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
#ifdef CszTest
                Csz::LI("constructor ReqCleBase");
#endif
				bzero(data,sizeof(data));
				//set id
				data[4]= T_id;
			}
            ~ReqCleBase()
            {
#ifdef CszTest
                Csz::LI("destructor ReqCleBase");
#endif
            }
			void SetParameter(int32_t T_index,int32_t T_begin,int32_t T_length);
			const char* GetSendData()const {return data;}
			const char* operator()(){return GetSendData();}
			int GetDataSize(){return 17;}
            void COutInfo();
	};
 
	//9.1 Request id= 6
	class Request
	{
		private:
			//impl
			ReqCleBase request;
		public:
			//set id
			Request():request(6)
            {
#ifdef CszTest
                Csz::LI("constructor Request");
#endif
            }
            ~Request()
            {
#ifdef CszTest
                Csz::LI("destructor Request");
#endif
            }
			void SetParameter(int32_t T_index,int32_t T_begin,int32_t T_length){request.SetParameter(T_index,T_begin,T_length);}
			const char* GetSendData()const {return request.GetSendData();}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize(){return request.GetDataSize();}
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
#ifdef CszTest
                Csz::LI("constructor Piece");
#endif
				//init prefix
				prefix_and_slice.resize(13,0);
				//set id
				prefix_and_slice[4]= 7;
			}
            ~Piece()
            {
#ifdef CszTest
                Csz::LI("destructor Piece");
#endif
            }
			void SetParameter(int32_t T_index,int32_t T_begin,PieceSliceType T_slice);
			const char* GetSendData()const {return prefix_and_slice.c_str();}
			const char* operator()(){return GetSendData();}
			int GetDataSize(){return prefix_and_slice.size();}
            void COutInfo();
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
			Cancle():cancle(8)
            {
#ifdef CszTest
                Csz::LI("constructor Cancle");
#endif
            }
            ~Cancle()
            {
#ifdef CszTest
                Csz::LI("destructor Cancle");
#endif
            }
			void SetParameter(int32_t T_index,int32_t T_begin,int32_t T_length){cancle.SetParameter(T_index,T_begin,T_length);}
			const char* GetSendData()const {return cancle.GetSendData();}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize(){return cancle.GetDataSize();}
	};

	//11.port id= 9
	class Port
	{
		private:
			char port[7];
		public:
			Port()
			{
#ifdef CszTest
                Csz::LI("constructor Port");
#endif
				bzero(port,sizeof(port));
				//set id
				port[4]= 9;
			}
            ~Port()
            {
#ifdef CszTest
                Csz::LI("destructor Port");
#endif
            }
			void SetParameter(int16_t T_port)
			{
                //2byte should htons
				*reinterpret_cast<int16_t*>(port+ 5)= htons(T_port);
				return ;
			}
			const char* GetSendData()const {return port;}
			const char* operator()(){return GetSendData();}
			//micro
			int GetDataSize(){return 7;}
	};
	
	//set singleton
	//local bitfield 
	class LocalBitField
	{
		//TODO valid bit length
		private:
			LocalBitField()
            {
#ifdef CszTest
                Csz::LI("constructor Local Bit Field");
#endif
            }
			~LocalBitField()
            {
#ifdef CszTest
                Csz::LI("destructor Local Bit Field");
#endif
            }
		public:
			static LocalBitField* GetInstance()
			{
				return butil::get_leaky_singleton<LocalBitField>();
			}
		private:
			friend void butil::GetLeakySingleton<LocalBitField>::create_leaky_singleton();
		private:
            BitField bit_field;
            //int32_t piece_length;
		public:
			bool GameOver()const{return bit_field.GameOver();}
			void RecvHave(int T_socket,int32_t T_index);
			void RecvBitField(int T_socket,const char* T_bit_field,const int T_eln);
            //int32_t GetPieceLength()const {return piece_length;}
            bool CheckBitField(int32_t T_index);
            void FillBitField(int32_t T_index);
            //init method
            void SetParameter(std::string T_bit_field,int32_t T_total)
            {
                bit_field.SetParameter(T_bit_field,T_total);
#ifdef CszTest
                Csz::LI("[Local Bit Field set parameter]INFO:");
                COutInfo();
#endif
                return ;
            }
			void ProgressBar(){bit_field.ProgressBar();return ;}
            const char* GetSendData()const
            {
                return bit_field.GetSendData();
            }
            int32_t GetDataSize()const {return bit_field.GetDataSize();}
			void COutInfo() const;
	};

	//DownSpeed
	class DownSpeed
	{
		private:
			DownSpeed()
            {
                pthread_rwlock_init(&lock,NULL);
#ifdef CszTest
                Csz::LI("constructor Down Speed");
#endif
            }
			~DownSpeed()
            {
                pthread_rwlock_destroy(&lock);
#ifdef CszTest
                Csz::LI("destructor Down Speed");
#endif
            }
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
			//socket | status |download speed
			struct DataType
            {
                DataType()= delete;
                DataType(int T_socket):socket(T_socket),total(0){}
                PeerStatus status; 
                int socket;
                uint32_t total;
            };
			//TODO hash table save status
			std::list<DownSpeed::DataType> queue;
            pthread_rwlock_t lock;
		public:
            //init total
			void AddSocket(const int T_socket)
			{
                DataType data(T_socket);
				queue.push_back(std::move(data));
                return ;
			}
			void AddTotal(const int T_socket,const uint32_t T_speed);
			void AddTotal(std::vector<std::pair<int,uint32_t>>& T_queue);
			//four and TODO check interesed
			std::vector<int> RetSocket();
			bool CheckSocket(const int T_socket);
            void ClearSocket(int T_socket);    
			void AmChoke(int T_socket);
			void AmUnChoke(int T_socket);
			void AmInterested(int T_socket);
			void AmUnInterested(int T_socket);
			void PrChoke(int T_socket);
			void PrUnChoke(int T_socket);
			void PrInterested(int T_socket);
			void PrUnInterested(int T_socket);
			void CalculateSpeed();
			void COutInfo();
	};

	//Need Piece
	class NeedPiece
	{
		private:
			NeedPiece()//:stop_runner(false)
            {
                pthread_rwlock_init(&id_lock,NULL);
#ifdef CszTest
                Csz::LI("constructor Need Piece");
#endif
            }
			~NeedPiece()
            {
                pthread_rwlock_destroy(&id_lock);
#ifdef CszTest
                Csz::LI("destructor Need Piece");
#endif       
            }
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
			//min priority or list sort
			//index->sockets|id
			struct DataType
            {
				DataType():index(-1),index_status(0){}
                int32_t index;
                //read 0x01 write 0x02
				uint8_t index_status;
                std::vector<std::pair<int,int>> queue;
            };
			//socket|id
			std::list<std::shared_ptr<NeedPiece::DataType>> index_queue;
            bthread::Mutex index_lock;
			//socket may be reuse
			//id->choke/unchoke and interested/uninterested
			//lazy not delete when socker closed
			pthread_rwlock_t id_lock;
			std::unordered_map<int,PeerStatus> id_queue;
/*
			//producer and curtomer
			bool stop_runner;
			bthread::ConditionVariable pop_cond;
			bthread::Mutex pop_mutex;
*/
		public:
			void PushNeed(const std::vector<int32_t>* T_index,const int T_socket,int T_id);
			void PushNeed(const int32_t T_index,int T_socket,int T_id);
			std::pair<int32_t,std::vector<int>> PopNeed();
            std::vector<int> PopPointNeed(int32_t T_index);
			bool Empty();
			//vector is id
			//init method
			void SocketMapId(int T_id);
			void AmChoke(int T_id);
			void AmUnChoke(int T_id);
			void AmInterested(int T_id);
			void AmUnInterested(int T_id);
			void PrChoke(int T_id);
			void PrUnChoke(int T_id);
			void PrInterested(int T_id);
			void PrUnInterested(int T_id);
            void ClearSocket(int T_id);
			void ClearIndex(const int T_index);
			void UnLockIndex(int T_index);
            void SendReq();
            static void* ASendReq(void* T_this);
            bool SetIndexW(int32_t T_index);
/*
			void Runner();
			void SR()
			{
				//std::unique_lock<bthread::Mutex> guard(pop_mutex);
				stop_runner= true;
				pop_cond.notify_one();
				return ;
			}
			bool RunnerStatus(){return stop_runner;}
*/
			void COutInfo();
        private:
            //TODO danger!!!
            PeerStatus* RetSocketStatus(int T_id);
	};

	//select & switch message type
	struct SelectSwitch
	{
		static fd_set rset_save;
#ifdef CszTest
        static int total;
#endif 
		struct Parameter
		{
			Parameter():socket(-1),len(0),buf(nullptr),cur_len(0)
            {
#ifdef  CszTest
                ++total;
                Csz::LI("[Select Switch parameter]->constructor->%s->%s->%d",__FILE__,__func__,__LINE__);
#endif
            }
			~Parameter()
            {
#ifdef CszTest
                --total;
                Csz::LI("[Select Switch parameter]->destructor->%s->%s->%d",__FILE__,__func__,__LINE__);
#endif
                if (buf!= nullptr) 
                {
                    delete[] buf;
#ifdef CszTest
                    Csz::LI("[Select Switch parameter]->delete");
#endif
                }
            }
			int socket;
			//not include id len
			int32_t len;
			char* buf;
			int32_t cur_len;
		};
		bool operator()();
		static void DKeepAlive(Parameter* T_data);
		static void DChoke(Parameter* T_data);/*id=0*/
		static void DUnChoke(Parameter* T_data);/*id= 1*/
		static void DInterested(Parameter* T_data);/*id= 2*/
		static void DUnInterested(Parameter* T_data);/*id=3 */
		static void DHave(Parameter* T_data);/*id= 4*/
		static void DBitField(Parameter* T_data);/*id= 5*/
		static void AsyncDBitField(Parameter* T_data);/*id= 5*/
		static void DRequest(Parameter* T_data);/*id= 6*/
        //lock socket,FD_CLR
		static void DPiece(Parameter* T_data);/*id= 7*/
		static void AsyncDPiece(Parameter* T_data);/*id= 7*/
		static void DCancle(Parameter* T_data);/*id= 8*/
		static void DPort(Parameter* T_data);/*id= 9*/
		//static void* RequestRuner(void*);
        private:
        static void _SendPiece(int T_socket,int32_t T_index,int32_t T_begin,int32_t T_length);
        static int8_t _LockPiece(int T_socket,int32_t T_index,int32_t T_begin,int32_t T_length);
	};
    
	struct BT
	{
		char data[512* 1024];
	};

    //memory manager
    class BitMemory
    {
        private:
            BitMemory()
            {
#ifdef CszTest
                Csz::LI("constructor Bit Memory");
#endif
            }
            ~BitMemory();
        public:
            static BitMemory* GetInstance()
            {
                return butil::get_leaky_singleton<BitMemory>();
            }
        private:
            friend void butil::GetLeakySingleton<BitMemory>::create_leaky_singleton();
        private:
            int32_t index_end;
            int32_t length_end;
			int32_t length_normal;
			bthread::Mutex mutex;
            struct DataType
            {
                int32_t cur_len;
                struct Inside
                {
                    Inside(char* T_buf,butil::ResourceId<BT>& T_id):buf(T_buf),id(T_id){}
                    char* buf;
                    butil::ResourceId<BT> id;
                };
                std::vector<Inside> data;
            };
            using DataTypeP= std::shared_ptr<DataType>;
            std::unordered_map<int32_t,DataTypeP> memory_pool;
        public:
            bool Write(int32_t T_index,int32_t T_begin,const char* T_buf,int32_t T_length);
            void Init(int32_t T_index_end,int32_t T_length_end,int32_t T_length_normal);
            void ClearIndex(int32_t T_index,int32_t T_len);
			void COutInfo();
        private:
            void _Clear();
            int32_t _Write(int32_t T_index);
    };
}

#endif //CszBITTORRENT_H
