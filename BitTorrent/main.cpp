#include <string>
//#include <fstream>
//#include <iostream>
#include <butil/files/file_path.h> //butil::file_path
#include <butil/files/file.h> //butil::file
//#include <bthread/bthread.h>
#include "CszBitTorrent.h"
#include "../Thread/CszSingletonThread.hpp"
#include "sha1.h"

int main(int argc,char** argv)
{
	if (argc< 2)
		return -1;
    butil::FilePath file_path(argv[1]);
    butil::File file(file_path,butil::File::FLAG_OPEN | butil::File::FLAG_READ);
    if (!file.IsValid())
    {
        Csz::ErrQuit("torrent file %s",butil::File::ErrorToString(file.error_details()).c_str());
        file.Close();
        return -1;
    }
	std::string data(file.GetLength()+ 1,0);
    if (file.GetLength()!= file.Read(0,&data[0],data.size()))
    {
        Csz::ErrQuit("read byte num!= %d",file.GetLength());
        return -1;  
    }
    file.Close();
    Csz::HandShake::GetInstance();
    Csz::TorrentFile::GetInstance();
    Csz::PeerManager::GetInstance();
    Csz::LocalBitField::GetInstance();
    Csz::NeedPiece::GetInstance();
    Csz::DownSpeed::GetInstance();
    Csz::BitMemory::GetInstance();
    Csz::SingletonThread<Csz::SelectSwitch::Parameter,THREADNUM>::GetInstance()->Init(4);
	Csz::Tracker tracker;
	//计算info hash val
	{
		auto info_flag= data.find("info");
		if (data.npos== info_flag)
		{
			Csz::ErrQuit("torrent file no info data\n");
			return -1;
		}
		auto info_num= Csz::GetDictLength()(data.c_str()+ info_flag+ 4,data.size()- info_flag- 4);
		//20*8= 160bit
		char info_buf[21]={0};
		Sha1(data.c_str()+ info_flag+ 4,info_num,(unsigned char*)info_buf);
        std::string info_hash= Csz::UrlEscape()(std::string(info_buf,20));
		tracker.SetInfoHash(info_hash);
        //set reserved|info hash|peer id
	    Csz::HandShake::GetInstance()->SetParameter(nullptr,info_buf,tracker.GetAmId().c_str());
	}

	{
        Csz::BDict match;
	    match.Decode(data);

	    match.ReadData("",Csz::TorrentFile::GetInstance());
    }

	Csz::TorrentFile::GetInstance()->GetTrackInfo(&tracker);

	Csz::LocalBitField::GetInstance()->SetParameter(std::string(Csz::TorrentFile::GetInstance()->GetIndexBitTotal(),0),
													Csz::TorrentFile::GetInstance()->GetIndexTotal());

    Csz::BitMemory::GetInstance()->Init(Csz::TorrentFile::GetInstance()->GetIndexEnd(),
										Csz::TorrentFile::GetInstance()->GetIndexEndLength(),
										Csz::TorrentFile::GetInstance()->GetIndexNormalLength());
    //60s time out
	Csz::PeerManager::GetInstance()->LoadPeerList(tracker.GetPeerList(60));
    auto start= time(NULL);
    //select
	{
	    while (Csz::SelectSwitch()()== false && !Csz::LocalBitField::GetInstance()->GameOver())
	    {
		    auto peer_list= tracker.GetPeerList(60);
			Csz::PeerManager::GetInstance()->LoadPeerList(peer_list);
		}
	}
    auto stop= time(NULL);
	std::cout<<"hello world:"<<stop- start<<"\n";
	//exit(1);
	return 0;
}
