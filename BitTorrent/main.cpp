#include <string>
#include <fstream>
#include <iostream>
#include "CszBitTorrent.h"
#include "../Thread/CszSingletonThread.hpp"
#include "sha1.h"

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
        std::string info_hash= Csz::UrlEscape()(std::string(info_buf,20));
		tracker.SetInfoHash(info_hash);
        //set reserved|info hash|peer id
	    Csz::HandShake::GetInstance()->SetParameter(nullptr,info_hash.c_str(),nullptr);
	}
	Csz::BDict match;
	match.Decode(data);

#ifdef CszTest
	printf("Dict Info:\n");
	match.COutInfo();
#endif

	match.ReadData("",Csz::TorrentFile::GetInstance());

#ifdef CszTest
	printf("Standard Info:\n");
	Csz::TorrentFile::GetInstance()->COutInfo();
#endif

	Csz::TorrentFile::GetInstance()->GetTrackInfo(&tracker);

#ifdef CszTest
	printf("Tracker extract info:\n");
	tracker.COutInfo();
#endif

	tracker.Connect();
	Csz::LocalBitField::GetInstance()->SetParameter(std::string(Csz::TorrentFile::GetInstance()->GetIndexTotal(),0));
	Csz::LocalBitField::GetInstance()->SetEndBit(Csz::TorrentFile::GetInstance()->GetEndBit());
    //60s time out
	Csz::PeerManager::GetInstance()->LoadPeerList(tracker.GetPeerList(60));
    //select
	{
		while (Csz::SelectSwitch()()== false)
		{
			auto peer_list= tracker.GetPeerList(60);
			Csz::PeerManager::GetInstance()->LoadPeerList(peer_list);
		}
	}
	return 0;
}
