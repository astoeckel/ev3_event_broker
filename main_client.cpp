/**
 *  EV3 Event Broker -- Talk to Lego Robots using UDP
 *  Copyright (C) 2019  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstring>
#include <random>

#include <unistd.h>

#include <ev3_event_broker/event_loop.hpp>
#include <ev3_event_broker/marshaller.hpp>
#include <ev3_event_broker/socket.hpp>
#include <ev3_event_broker/source_id.hpp>
#include <ev3_event_broker/timer.hpp>

using namespace ev3_event_broker;

struct Listener : public Demarshaller::Listener {
	socket::Address ip;

	void on_position_sensor(
	    const Demarshaller::Header &header,
	    const Demarshaller::PositionSensor &position) override {
		printf(
		    "{"
		    "\"source\":\"%.*s:%.*s\","
		    "\"ip\":\"%d.%d.%d.%d\","
		    "\"port\":%d,"
		    "\"seq\":%d,"
		    "\"type\":\"position\","
		    "\"sensor\":\"%.*s\","
		    "\"position\":%d"
		    "}\n",
		    int(sizeof(header.source_name)), header.source_name,
		    int(sizeof(header.source_hash)), header.source_hash, ip.a, ip.b,
		    ip.c, ip.d, ip.port, header.sequence,
		    int(sizeof(position.device_name)), position.device_name,
		    position.position);
	}
};

int main(int argc, char *argv[]) {
	SourceId source_id("nengo");
	Listener listener;

	uint16_t port = 4721;
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::UDP sock(listen_address);

	EventLoop()
	    .register_event(sock,
	                    [&]() -> bool {
		                    socket::Address addr;
		                    socket::Message msg;
		                    if (!sock.recv(addr, msg)) {
			                    return false;
		                    }
		                    listener.ip = addr;
		                    Demarshaller().parse(listener, msg.buf(),
		                                         msg.size());
		                    return true;
	                    })
	    .run();

	return 0;
}

