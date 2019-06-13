.PHONY: all clean

CXX ?= g++

all: ev3_broker_server ev3_broker_client

ev3_broker_server: \
		main_server.cpp \
		ev3_event_broker/*.hpp \
		ev3_event_broker/*.cpp
	$(CXX) $(CXXFLAGS) -Wno-psabi -Wall -Wextra -g -O0 -I. -o ev3_broker_server \
		main_server.cpp ev3_event_broker/*.cpp

ev3_broker_client: \
		main_client.cpp \
		ev3_event_broker/*.hpp \
		ev3_event_broker/*.cpp
	$(CXX) $(CXXFLAGS) -Wno-psabi -Wall -Wextra -g -O0 -I. -o ev3_broker_client \
		main_client.cpp ev3_event_broker/*.cpp

clean:
	rm -f ev3_broker_server ev3_broker_client
