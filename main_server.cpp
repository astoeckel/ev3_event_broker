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

class Listener : public Demarshaller::Listener {
private:
	SourceId &m_source_id;
	Motors &m_motors;

public:
	Listener(SourceId &source_id, Motors &motors)
	    : m_source_id(m_source_id), m_motors(motors) {}

	/**
	 * Implementation of the filter() function. Discards messages originating
	 * from this device.
	 */
	bool filter(const Demarshaller::Header &header) override {
		return (strcmp(header.source_name, m_source_id.name()) != 0) ||
		       (strcmp(header.source_hash, m_source_id.hash()) != 0);
	}

	void on_set_duty_cycle(
	    const Demarshaller::Header &,
	    const Demarshaller::SetDutyCycle &set_duty_cycle) override {
		TachoMotor *motor = m_motors.find(set_duty_cycle.device_name);
		if (motor) {
			motor->set_duty_cycle(set_duty_cycle.duty_cycle);
		}
	}

	void on_reset(const Demarshaller::Header &) override {
		for (TachoMotor &motor : m_motors.motors()) {
			motor.reset();
		}
	}
};

int main(int argc, char *argv[]) {
	// Create the UDP socket and setup all addresses
	uint16_t port = 4721;
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::Address broadcast_address(255, 255, 255, 255, port);
	socket::UDP sock(listen_address);

	// Fetch all motors
	Motors motors;

	// Create a marshaller instance with a randomized source_id and connect it
	// to the socket
	SourceId source_id("EV3");
	Marshaller marshaller(
	    [&](const uint8_t *buf, size_t buf_size) -> bool {
		    socket::Message msg(buf, buf_size);
		    sock.send(broadcast_address, msg);
		    return true;
	    },
	    source_id.name(), source_id.hash());

	// Setup the demarshaller for incoming messages
	Listener listener(source_id, motors);
	Demarshaller demarshaller;

	// Repeatedly send the position of all connected motors
	Timer position_timer(10);  // interval: 10ms
	auto handle_position_timer = [&]() -> bool {
		// Mark the timer event as handled
		position_timer.consume_event();

		// Send the motor position
		try {
			for (const auto &motor : motors.motors()) {
				marshaller.write_position_sensor(motor.name(),
				                                 motor.get_position());
			}
			marshaller.flush();
		} catch (std::system_error &e) {
			// There was an error reading from the motors, scan for new motors
			motors.rescan();
		}
		return bool(marshaller);
	};

	// Rescan available motors from time to time
	Timer rescan_timer(1000);  // interval: 5s
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
		demarshaller.parse(listener, msg.buf(), msg.size());
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

