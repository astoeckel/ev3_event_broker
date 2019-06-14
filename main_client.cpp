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
	Listener listener;

	uint16_t port = 4721;
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::Address target_address(192, 168, 178, 100, port);
	socket::UDP sock(listen_address);

	SourceId source_id("nengo");
	Marshaller marshaller(
	    [&](const uint8_t *buf, size_t buf_size) -> bool {
		    socket::Message msg(buf, buf_size);
		    sock.send(target_address, msg);
		    return true;
	    },
	    source_id.name(), source_id.hash());

	auto handle_sock = [&]() -> bool {
		                    socket::Address addr;
		                    socket::Message msg;
		                    if (!sock.recv(addr, msg)) {
			                    return false;
		                    }
		                    listener.ip = addr;
		                    Demarshaller().parse(listener, msg.buf(),
		                                         msg.size());
		                    return true;
	                    };

	Timer timer(10);
	int dir = 1;
	int duty_cycle = 0;
	auto handle_timer = [&]() -> bool {
		timer.consume_event();

		duty_cycle += dir;
		if (duty_cycle >= 100 || duty_cycle <= -100) {
			//dir *= -1;
			marshaller.write_reset();
		} else {
			marshaller.write_set_duty_cycle("motor_outA", duty_cycle);
			marshaller.write_set_duty_cycle("motor_outB", -duty_cycle);
		}
		marshaller.flush();
		return true;
	};

	EventLoop()
	    .register_event(sock, handle_sock)
	    .register_event(timer, handle_timer)
	    .run();

	return 0;
}

