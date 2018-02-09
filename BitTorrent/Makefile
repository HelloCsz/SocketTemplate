include ../config.mk

TARGET= BitTorrent
HDR_DIR= $(SRC)/Error $(SRC)/Web $(SRC)/Sock

SOURCES= $(SRC)/BitTorrent/main.cpp\
		 $(SRC)/BitTorrent/CszBInt.cpp\
		 $(SRC)/BitTorrent/CszBStr.cpp\
		 $(SRC)/BitTorrent/CszBList.cpp\
		 $(SRC)/BitTorrent/CszBDict.cpp\
		 $(SRC)/BitTorrent/CszTorrentFile.cpp\
		 $(SRC)/BitTorrent/CszTracker.cpp\
		 $(SRC)/BitTorrent/CszTrackerInfo.cpp\
		 $(SRC)/BitTorrent/CszGetDictLength.cpp\
		 $(SRC)/BitTorrent/sha1.c\
		 $(SRC)/Error/CszError.cpp\
		 $(SRC)/Web/CszHttpRequest.cpp\
		 $(SRC)/Web/CszHttpResponse.cpp\
		 $(SRC)/Web/CszUrlEscape.cpp\
		 $(SRC)/Web/CszCacheRegio.cpp\
		 $(SRC)/Sock/CszTcpConnect.cpp\
		 $(SRC)/Sock/CszFcntl.cpp\
		 $(SRC)/Sock/CszCreateSocket.cpp\
		 $(SRC)/Sock/CszSocketHostServNull.cpp

OBJECTS= $(addsuffix .o, $(basename $(SOURCES)))

LIB_DIR=

HDR_INCLUDE= $(addprefix -I, $(HDR_DIR))
LIB_LINK= $(addprefix -L, $(LIB_DIR))

CXXFLAGS= -std=c++11 -g -W
STATICLINK_FLAGS= #-lgflags
DYNAMICLINK_FLAGS= #-pthread

all:$(TARGET)

$(TARGET):$(OBJECTS)
	@echo "Linking $@"
	@$(CXX) $(LIB_LINK) -Xlinker "-(" $(OBJECTS) -Wl,-Bstatic $(STATICLINK_FLAGS) -Wl,-Bdynamic -Xlinker "-)" $(DYNAMICLINK_FLAGS) -o $@

clean:
	@echo "Cleaning BitTorrent"
	@rm -frv $(OBJECTS) $(TARGET)
%.o:%.cpp
	@echo "Compiling $@"
	@$(CXX) -c $(HDR_INCLUDE) $(CXXFLAGS) $< -o $@
%.o:%.c
	@echo "Compiling $@"
	@$(CXX) -c $(HDR_INCLUDE) $(CXXFLAGS) $< -o $@
