#  EV3 Event Broker -- Talk to Lego Robots using UDP
#  Copyright (C) 2019  Andreas Stöckel
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

CXX?=g++ # Use g++ if no other compiler is specified
BUILD?=release

CPPFLAGS+=-std=c++14
CPPFLAGS+=-Wno-psabi # Silence warning when compiling for ARM
CPPFLAGS+=-Wall -Wextra -pedantic
CPPFLAGS+=-I.
CPPFLAGS+=-Ilib/json/src
CPPFLAGS+=-DNDEBUG -g -O3

OBJDIR=obj
MKOBJ=$(CXX) $(CPPFLAGS) $(FLAGS) -c

all: ev3_broker_client ev3_broker_server

clean:
	rm -f $(OBJDIR)/ev3_event_broker/*.o
	rm -f $(OBJDIR)/*.o
	rm -f ev3_broker_client ev3_broker_server

$(OBJDIR)/ev3_event_broker/argparse.o: \
		ev3_event_broker/argparse.cpp \
		ev3_event_broker/argparse.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ ev3_event_broker/argparse.cpp

$(OBJDIR)/ev3_event_broker/event_loop.o: \
		ev3_event_broker/event_loop.cpp \
		ev3_event_broker/error.hpp \
		ev3_event_broker/event_loop.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ ev3_event_broker/event_loop.cpp

$(OBJDIR)/ev3_event_broker/marshaller.o: \
		ev3_event_broker/marshaller.cpp \
		ev3_event_broker/marshaller.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ ev3_event_broker/marshaller.cpp

$(OBJDIR)/ev3_event_broker/motors.o: \
		ev3_event_broker/motors.cpp \
		ev3_event_broker/error.hpp \
		ev3_event_broker/motor.hpp \
		ev3_event_broker/motors.hpp \
		ev3_event_broker/tacho_motor.hpp \
		ev3_event_broker/virtual_motor.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ ev3_event_broker/motors.cpp

$(OBJDIR)/ev3_event_broker/socket.o: \
		ev3_event_broker/socket.cpp \
		ev3_event_broker/error.hpp \
		ev3_event_broker/socket.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ $<

$(OBJDIR)/ev3_event_broker/source_id.o: \
		ev3_event_broker/source_id.cpp \
		ev3_event_broker/source_id.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ ev3_event_broker/source_id.cpp

$(OBJDIR)/ev3_event_broker/tacho_motor.o: \
		ev3_event_broker/tacho_motor.cpp \
		ev3_event_broker/common.hpp \
		ev3_event_broker/error.hpp \
		ev3_event_broker/tacho_motor.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ ev3_event_broker/tacho_motor.cpp

$(OBJDIR)/ev3_event_broker/virtual_motor.o: \
		ev3_event_broker/virtual_motor.cpp \
		ev3_event_broker/common.hpp \
		ev3_event_broker/error.hpp \
		ev3_event_broker/tacho_motor.hpp \
		ev3_event_broker/virtual_motor.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ ev3_event_broker/virtual_motor.cpp

$(OBJDIR)/main_client.o: \
		main_client.cpp \
		ev3_event_broker/argparse.hpp \
		ev3_event_broker/error.hpp \
		ev3_event_broker/event_loop.hpp \
		ev3_event_broker/marshaller.hpp \
		ev3_event_broker/socket.hpp \
		ev3_event_broker/source_id.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ main_client.cpp

$(OBJDIR)/main_server.o: \
		main_server.cpp \
		ev3_event_broker/argparse.hpp \
		ev3_event_broker/event_loop.hpp \
		ev3_event_broker/marshaller.hpp \
		ev3_event_broker/motor.hpp \
		ev3_event_broker/tacho_motor.hpp \
		ev3_event_broker/virtual_motor.hpp \
		ev3_event_broker/socket.hpp \
		ev3_event_broker/source_id.hpp
	mkdir -pv $$(dirname $@)
	$(MKOBJ) -o $@ main_server.cpp


ev3_broker_client: \
		$(OBJDIR)/ev3_event_broker/argparse.o \
		$(OBJDIR)/ev3_event_broker/event_loop.o \
		$(OBJDIR)/ev3_event_broker/marshaller.o \
		$(OBJDIR)/ev3_event_broker/socket.o \
		$(OBJDIR)/ev3_event_broker/source_id.o \
		$(OBJDIR)/main_client.o
	$(CXX) $(LDFLAGS) $^ -o $@

ev3_broker_server: \
		$(OBJDIR)/ev3_event_broker/argparse.o \
		$(OBJDIR)/ev3_event_broker/event_loop.o \
		$(OBJDIR)/ev3_event_broker/marshaller.o \
		$(OBJDIR)/ev3_event_broker/socket.o \
		$(OBJDIR)/ev3_event_broker/source_id.o \
		$(OBJDIR)/ev3_event_broker/motors.o \
		$(OBJDIR)/ev3_event_broker/tacho_motor.o \
		$(OBJDIR)/ev3_event_broker/virtual_motor.o \
		$(OBJDIR)/main_server.o
	$(CXX) $(LDFLAGS) $^ -o $@
