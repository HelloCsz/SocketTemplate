include ../config.mk

TARGET= BitTorrent
HDR_DIR= $(SRC)/Error $(SRC)/Web $(SRC)/Sock

SOURCES= $(SRC)/BitTorrent/main.cpp\
		 $(SRC)/BitTorrent/sha1.c\
		 $(SRC)/BitTorrent/CszBInt.cpp\
		 $(SRC)/BitTorrent/CszBStr.cpp\
		 $(SRC)/BitTorrent/CszBList.cpp\
		 $(SRC)/BitTorrent/CszBDict.cpp\
		 $(SRC)/BitTorrent/CszTorrentFile.cpp\
		 $(SRC)/BitTorrent/CszTracker.cpp\
		 $(SRC)/BitTorrent/CszTrackerInfo.cpp\
		 $(SRC)/BitTorrent/CszGetDictLength.cpp\
		 $(SRC)/BitTorrent/CszHandShake.cpp\
		 $(SRC)/BitTorrent/CszBitField.cpp\
		 $(SRC)/BitTorrent/CszDownSpeed.cpp\
		 $(SRC)/BitTorrent/CszNeedPiece.cpp\
		 $(SRC)/BitTorrent/CszPeerManager.cpp\
		 $(SRC)/BitTorrent/CszLocalBitField.cpp\
		 $(SRC)/BitTorrent/CszPiece.cpp\
		 $(SRC)/BitTorrent/CszReqCleBase.cpp\
		 $(SRC)/BitTorrent/CszSelectSwitch.cpp\
		 $(SRC)/BitTorrent/CszBitMemory.cpp\
		 $(SRC)/Error/CszError.cpp\
		 $(SRC)/Web/CszHttpRequest.cpp\
		 $(SRC)/Web/CszHttpResponse.cpp\
		 $(SRC)/Web/CszUrlEscape.cpp\
		 $(SRC)/Web/CszCacheRegio.cpp\
		 $(SRC)/Sock/CszTcpConnect.cpp\
		 $(SRC)/Sock/CszFcntl.cpp\
		 $(SRC)/Sock/CszCreateSocket.cpp\
		 $(SRC)/Sock/CszSocketHostServNull.cpp\
		 $(SRC)/Sock/CszRecvTime_us.cpp\
		 $(SRC)/Sock/CszClose.cpp\
		 $(SRC)/Signal/CszSignal.cpp

OBJECTS= $(addsuffix .o, $(basename $(SOURCES)))
OBJECTS+= $(PROTOS:.proto=.pb.o)

HDR_DIR+= $(BRPC_SRC_DIR)/output/include 
#HDR_DIR+= /usr/include	
HDR_DIR+= $(PROTOS_DIR)	

LIB_DIR= $(BRPC_SRC_DIR)/output/lib	
#LIB_DIR+= /usr/lib

HDR_INCLUDE= $(addprefix -I, $(HDR_DIR))
LIB_LINK= $(addprefix -L, $(LIB_DIR))

CXXFLAGS= -std=c++11 -O2 #-W -g -fsanitize=address #-lasan #-fno-omit-frame-pointer
# -Winit-self -Wlogical-op -Weffc++  -fsanitize=address -lasan -fsanitize-leak -Warray-bounds -Wtype-limits
STATICLINK_FLAGS= -lgflags -lprotobuf -lbrpc #-fsanitize=address
DYNAMICLINK_FLAGS= -pthread -lssl 

.PHONY:
all:$(TARGET) 

brpc-conf:$(BRPC_SRC_DIR)/config_brpc.sh
	@echo "Configuring brpc"
	@cd $(BRPC_SRC_DIR) && sh ./config_brpc.sh --headers=/usr/include --libs=/usr/lib

brpc:$(BRPC_SRC_DIR)/Makefile brpc-conf
	@echo "Building brpc"
	@make -C $(BRPC_SRC_DIR)

$(TARGET):brpc $(OBJECTS)
	@echo "Linking $@"
	@$(CXX) $(CXXFLAGS) $(LIB_LINK) -Xlinker "-(" $(OBJECTS) -Wl,-Bstatic $(STATICLINK_FLAGS) -Wl,-Bdynamic -Xlinker "-)" $(DYNAMICLINK_FLAGS) -o $@ #-llsan

.PHONY:
clean:
	@echo "Cleaning BitTorrent"
	@rm -frv $(OBJECTS) $(TARGET)
	@echo "Cleaning brpc"	
#	@make -C $(BRPC_SRC_DIR) clean 

%.pb.cc %.pb.h: %.proto
	@echo "Generating $@"
	@(PROTOC) --cpp_out=$(PROTOS_DIR) --proto_path=$(PROTOS_DIR) $(PROTOC_EXTRA_ARGS) $<

%.o:%.cpp
	@echo "Compiling $@"
	@$(CXX) -c $(HDR_INCLUDE) $(CXXFLAGS) $< -o $@ #-llsan

%.o:%.c
	@echo "Compiling $@"
	@$(CXX) -c $(HDR_INCLUDE) $(CXXFLAGS) $< -o $@ #-llsan
