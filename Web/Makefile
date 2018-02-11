include ../config.mk

TARGET= Web
HDR_DIR= $(SRC)/Error $(SRC)/Web $(SRC)/Sock

SOURCES= ./main.cpp\
		 ./CszHttpRequest.cpp\
		 ./CszHttpResponse.cpp\
	     ./CszCacheRegio.cpp\
		 $(SRC)/Error/CszError.cpp\
		 $(SRC)/Sock/CszTcpConnect.cpp\
		 $(SRC)/Sock/CszSocketNtoPHost.cpp\

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
