far_objects= CszError.o CszTcpConnect.o CszSocketNtoPHost.o
far_heads= CszError.h CszSocket.h

curr_objects= main.o CszHttpRequest.o CszHttpResponse.o CszCacheRegio.o
curr_heads= CszWeb.h

flag= -std=c++11
vpath % ../Error ../Sock

Web:$(curr_objects) $(curr_heads) $(far_objects) $(far_heads)
	g++ $(flag) -o web $(curr_objects) $(far_objects)

$(curr_objects):%.o:%.cpp
	g++ $(flag) -c $<
$(far_objects):%.o:%.cpp
	g++ $(flag) -c $< -I $@

.PHONY:clean
clean:
	rm *.o
