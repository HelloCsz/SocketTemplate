far_objects= CszError.o CszHttpRequest.o CszHttpResponse.o CszUrlEscape.o CszTcpConnect.o CszCacheRegio.o\
			 CszFcntl.o CszCreateSocket.o CszSocketHostServNull.o	
far_heads= CszError.h CszWeb.h CszSocket.h

curr_objects= main.o CszBInt.o CszBStr.o CszBList.o CszBDict.o CszTorrentFile.o CszTracker.o CszTrackerInfo.o CszGetDictCount.o
curr_heads= CszBitTorrent.h	CszNonCopyAble.hpp sha1.h

flag= -std=c++11 -W

vpath % ../Web ../Error ../Sock

BitTorrent:$(curr_objects) $(far_objects) sha1.o $(curr_heads) $(far_heads)
	g++ $(flag) -o BitTorrent $(curr_objects) $(far_objects) sha1.o 
sha1.o: sha1.h
	g++ $(flag) -c sha1.c -o sha1.o
$(curr_objects): %.o:%.cpp	
	g++ $(flag) -c $< -o $@
$(far_objects):%.o:%.cpp	
	g++ $(flag) -c $< -I ./$@
.PHONY:clean	
clean:
	rm *.o

