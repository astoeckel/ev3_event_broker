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
#include <ev3_event_broker/tacho_motor.hpp>
#include <ev3_event_broker/timer.hpp>

using namespace ev3_event_broker;

int main(int argc, char *argv[]) {
	uint16_t port = 4721;
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::Address broadcast_address(192, 168, 178, 255, port);
	socket::UDP sock(listen_address);

	SourceId source_id("ev3");

	Timer timer(1);

	Marshaller marshaller(
	    [&](const uint8_t *buf, size_t buf_size) -> bool {
		    printf("Send motor_D position!\n");
		    socket::Message msg(buf, buf_size);
		    sock.send(broadcast_address, msg);
		    return true;
	    },
	    source_id.name(), source_id.hash());

	TachoMotor motor_D("/sys/class/tacho-motor/motor1");

	EventLoop()
	    .register_event(timer,
	                    [&]() -> bool {
		                    // Mark the timer event as handled
		                    timer.consume_event();

		                    // Send the motor position
		                    marshaller.write_position_sensor(
		                        "motor_D", motor_D.get_position());
		                    marshaller.flush();
		                    return bool(marshaller);
	                    })
	    .register_event(sock,
	                    [&]() -> bool {
		                    socket::Address addr;
		                    socket::Message msg;
		                    if (!sock.recv(addr, msg)) {
			                    return false;  // Socket was closed
		                    }
		                    return true;
	                    })
	    .run();

	return 0;
}

