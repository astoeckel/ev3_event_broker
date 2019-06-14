.PHONY: all clean

CXX ?= g++
FLAGS = -Wno-psabi -Wall -Wextra -s -O0 -I. -Ijson/src

all: ev3_broker_server ev3_broker_client

ev3_broker_server: \
		main_server.cpp \
		ev3_event_broker/*.hpp \
		ev3_event_broker/*.cpp
	$(CXX) $(CXXFLAGS) $(FLAGS) -o ev3_broker_server \
		main_server.cpp ev3_event_broker/*.cpp

ev3_broker_client: \
		main_client.cpp \
		ev3_event_broker/*.hpp \
		ev3_event_broker/*.cpp
	$(CXX) $(CXXFLAGS) $(FLAGS) -o ev3_broker_client \
		main_client.cpp ev3_event_broker/*.cpp

clean:
	rm -f ev3_broker_server ev3_broker_client
