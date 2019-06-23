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

#include <ev3_event_broker/argparse.hpp>
#include <ev3_event_broker/event_loop.hpp>
#include <ev3_event_broker/marshaller.hpp>
#include <ev3_event_broker/motors.hpp>
#include <ev3_event_broker/socket.hpp>
#include <ev3_event_broker/source_id.hpp>

using namespace ev3_event_broker;

class Listener : public Demarshaller::Listener {
private:
	bool &m_conflict;
	SourceId &m_source_id;
	Motors &m_motors;

public:
	Listener(bool &conflict, SourceId &source_id, Motors &motors)
	    : m_conflict(conflict), m_source_id(source_id), m_motors(motors)
	{
	}

	/**
	 * Implementation of the filter() function. Discards messages originating
	 * from this device.
	 */
	bool filter(const Demarshaller::Header &header) override
	{
		return (strcmp(header.source_name, m_source_id.name()) != 0) ||
		       (strcmp(header.source_hash, m_source_id.hash()) != 0);
	}

	void on_set_duty_cycle(
	    const Demarshaller::Header &,
	    const Demarshaller::SetDutyCycle &set_duty_cycle) override
	{
		try {
			Motor *motor = m_motors.find(set_duty_cycle.device_name);
			if (motor) {
				motor->set_duty_cycle(set_duty_cycle.duty_cycle);
			}
		}
		catch (std::system_error &) {
			m_motors.rescan();
		}
	}

	void on_reset(const Demarshaller::Header &) override
	{
		for (auto &motor : m_motors.motors()) {
			try {
				motor->reset();
			}
			catch (std::system_error &) {
				// Do nothing here, just continue resetting
			}
		}
	}

	void on_heartbeat(const Demarshaller::Header &header) override
	{
		m_conflict |= (strcmp(header.source_name, m_source_id.name()) == 0);
	}
};

int main(int argc, const char *argv[])
{
	uint16_t port;
#ifndef VIRTUAL_MOTORS
	std::string device_name = "EV3";
#else
	std::string device_name = "EV3_VIRT";
#endif

	Argparse(argv[0],
	         "Dispatches incoming EV3 Event Broker messages as JSON on stdout "
	         "and reads JSON commands from stdin.")
	    .add_arg("port", "The UDP port to listen on", "4721",
	             [&](const char *value) -> bool {
		             char *endptr;
		             port = strtol(value, &endptr, 10);
		             return *endptr == '\0';
	             })
	    .add_arg("name", "Name of this device", device_name.c_str(),
	             [&](const char *value) -> bool {
		             device_name = value;
		             return true;
	             })
	    .parse(argc, argv);

	// Create the UDP socket and setup all addresses
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::Address broadcast_address(255, 255, 255, 255, port);
	socket::UDP sock(listen_address);
	fprintf(stderr, "Listening on %d.%d.%d.%d:%d as \"%s\"...\n",
	        listen_address.a, listen_address.b, listen_address.c,
	        listen_address.d, port, device_name.c_str());

	// Fetch all motors
	Motors motors;

	// Create a marshaller instance with a randomized source_id and connect it
	// to the socket
	SourceId source_id(device_name.c_str());
	Marshaller marshaller(
	    [&](const uint8_t *buf, size_t buf_size) -> bool {
		    socket::Message msg(buf, buf_size);
		    sock.send(broadcast_address, msg);
		    return true;
	    },
	    source_id.name(), source_id.hash());

	// Setup the demarshaller for incoming messages, create a variable
	// indicating whether there was a conflict or not.
	bool conflict = false;
	Listener listener(conflict, source_id, motors);
	Demarshaller demarshaller;

	// Periodically send all sensor data
	bool sensor_broadcast_enabled = false;
	auto handle_sensor_timer = [&]() -> bool {
		if (!sensor_broadcast_enabled) {
			return true;
		}
		try {
			for (const auto &motor : motors.motors()) {
				marshaller.write_position_sensor(motor->name(),
				                                 motor->get_position());
			}
			marshaller.flush();
		}
		catch (std::system_error &e) {
			motors.rescan();
		}
		return bool(marshaller);
	};

	// Rescan available motors from time to time
	auto handle_rescan_timer = [&]() -> bool {
		motors.rescan();
		return true;
	};

	// Timer sending a regular heartbeat
	int n_heartbeat = 0;
	auto handle_hearbeat_timer = [&]() -> bool {
		n_heartbeat++;
		if (!sensor_broadcast_enabled && conflict) {
			fprintf(stderr,
			        "ERROR: Another device is already active with name \"%s\". Aborting.\n",
			        source_id.name());
			exit(EXIT_FAILURE);
		}
		else if (n_heartbeat > 4 && !conflict) {
			sensor_broadcast_enabled = true;
		}
		marshaller.write_heartbeat();
		marshaller.flush();
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
	    .register_timer(10, handle_sensor_timer)
	    .register_timer(1000, handle_rescan_timer)
	    .register_timer(250, handle_hearbeat_timer)
	    .register_event(sock, handle_sock)
	    .run();

	return 0;
}

