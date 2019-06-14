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
#include <system_error>

#include <ev3_event_broker/event_loop.hpp>
#include <ev3_event_broker/marshaller.hpp>
#include <ev3_event_broker/motors.hpp>
#include <ev3_event_broker/socket.hpp>
#include <ev3_event_broker/source_id.hpp>
#include <ev3_event_broker/timer.hpp>

using namespace ev3_event_broker;

int main(int argc, char *argv[]) {
	// Create the UDP socket and setup all addresses
	uint16_t port = 4721;
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::Address broadcast_address(255, 255, 255, 255, port);
	socket::UDP sock(listen_address);

	// Fetch all motors
	Motors motors;
	if (motors.motors().size() > 0) {
		printf("Found %ld motors\n", motors.motors().size());
		for (const auto &item : motors.motors()) {
			printf("\t%s\n", item.first.c_str());
		}
	} else {
		printf("Warning: no attached motors found\n");
	}

	// Create a marshaller instance with a randomized source_id and connect it
	// to the socket
	SourceId source_id("ev3");
	Marshaller marshaller(
	    [&](const uint8_t *buf, size_t buf_size) -> bool {
		    socket::Message msg(buf, buf_size);
		    sock.send(broadcast_address, msg);
		    return true;
	    },
	    source_id.name(), source_id.hash());

	// Repeatedly send the position of all connected motors
	Timer position_timer(10);  // interval: 10ms
	auto handle_position_timer = [&]() -> bool {
		// Mark the timer event as handled
		position_timer.consume_event();

		// Send the motor position
		try {
			for (const auto &item : motors.motors()) {
				marshaller.write_position_sensor(item.first.c_str(),
					                             item.second.get_position());
			}
			marshaller.flush();
		} catch (std::system_error &e) {
			// There was an error reading from the motors, scan for new motors
			motors.rescan();
		}
		return bool(marshaller);
	};

	// Rescan available motors from time to time
	Timer rescan_timer(1000); // interval: 5s
	auto handle_rescan_timer = [&]() -> bool {
		rescan_timer.consume_event();
		motors.rescan();
		return true;
	};

	// Handle incoming commands
	auto handle_sock = [&]() -> bool {
		socket::Address addr;
		socket::Message msg;
		if (!sock.recv(addr, msg)) {
			return false;  // Socket was closed
		}
		return true;
	};

	// Run the event loop
	EventLoop()
	    .register_event(position_timer, handle_position_timer)
	    .register_event(rescan_timer, handle_rescan_timer)
	    .register_event(sock, handle_sock)
	    .run();

	return 0;
}

