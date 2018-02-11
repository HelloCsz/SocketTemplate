#include "CszBitTorrent.h"
#include <string>
#include <fstream>
#include <iostream>

int main(int argc,char** argv)
{
	if (argc< 2)
		return 0;
	std::ifstream file(argv[1]);
	if (!file.is_open())
	{
		printf("%s open faiure\n",argv[1]);
		return 0;
	}
	std::string data((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
#ifdef CszTest
	printf("file size:%lu\n",data.size());
#endif
	file.close();
	Csz::Tracker tracker;
	//计算info hash val
	{
		auto info_flag= data.find("info");
		if (data.npos== info_flag)
		{
			Csz::ErrQuit("torrent file no info data\n");
		}
		auto info_num= Csz::GetDictLength()(data.c_str()+ info_flag+ 4,data.size()- info_flag- 4);
		//20*8= 160bit
		char info_buf[21]={0};
		Sha1(data.c_str()+ info_flag+ 4,info_num,(unsigned char*)info_buf);
		tracker.SetInfoHash(Csz::UrlEscape()(std::string(info_buf,20)));
	}
	Csz::BDict match;
	match.Decode(data);
#ifdef CszTest
	printf("Dict Info:\n");
	match.COutInfo();
#endif
	Csz::TorrentFile torrent_file;
	match.ReadData("",&torrent_file);
#ifdef CszTest
	printf("Standard Info:\n");
	torrent_file.COutInfo();
#endif
	torrent_file.GetTrackInfo(&tracker);
#ifdef CszTest
	printf("Tracker extract info:\n");
	tracker.COutInfo();
#endif
	Csz::HttpRequest request;
	request.SetHeader("Connection","Keep-alive");
	request.SetHeader("User-Agent","Super Max");
	request.SetHeader("Accept","text/html");
	Csz::HttpResponse response;
	std::shared_ptr<Csz::CacheRegio> cache= Csz::CacheRegio::GetSingleton();
	{
		auto id1= time(NULL);
		auto id2= std::rand()%100000000+ 1000000000;
		std::string id(std::to_string(id1));
		id.append(std::to_string(id2));
		std::string parameter("peer_id=");
		parameter.append(Csz::UrlEscape()(id));
		tracker.SetParameter(std::move(parameter));
		parameter.assign("port=54321");
		tracker.SetParameter(std::move(parameter));
		parameter.assign("compact=1");
		tracker.SetParameter(std::move(parameter));
		parameter.assign("uploaded=0");
		tracker.SetParameter(std::move(parameter));
		parameter.assign("downloaded=0");
		tracker.SetParameter(std::move(parameter));
		parameter.assign("left=");
		parameter.append(std::to_string(torrent_file.GetTotal()));
		tracker.SetParameter(std::move(parameter));
		parameter.assign("event=started");
		tracker.SetParameter(std::move(parameter));
		parameter.assign("numwant=50");
		tracker.SetParameter(std::move(parameter));
	}
	tracker.Connect();
	tracker.GetPeerList(&request,&response,cache,60);
	return 0;
}
