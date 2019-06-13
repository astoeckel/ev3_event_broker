.PHONY: all clean

CXX ?= g++

all: ev3_broker

ev3_broker: \
		main.cpp \
		ev3_event_broker/common.hpp \
		ev3_event_broker/error.hpp \
		ev3_event_broker/event_loop.cpp \
		ev3_event_broker/event_loop.hpp \
		ev3_event_broker/socket.cpp \
		ev3_event_broker/socket.hpp \
		ev3_event_broker/tacho_motor.cpp \
		ev3_event_broker/tacho_motor.hpp \
		ev3_event_broker/timer.cpp \
		ev3_event_broker/timer.hpp
	$(CXX) -Wall -Wextra -s -O3 -I. -o ev3_broker \
		main.cpp \
		ev3_event_broker/event_loop.cpp \
		ev3_event_broker/socket.cpp \
		ev3_event_broker/tacho_motor.cpp \
		ev3_event_broker/timer.cpp

clean:
	rm -f ev3_broker
