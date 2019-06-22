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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>

#include <fcntl.h>
#include <unistd.h>

#include <json.hpp>

#include <ev3_event_broker/argparse.hpp>
#include <ev3_event_broker/error.hpp>
#include <ev3_event_broker/event_loop.hpp>
#include <ev3_event_broker/marshaller.hpp>
#include <ev3_event_broker/socket.hpp>
#include <ev3_event_broker/source_id.hpp>

using namespace nlohmann;
using namespace ev3_event_broker;

class Listener : public Demarshaller::Listener {
private:
	SourceId &m_source_id;
	socket::Address &m_source_address;

public:
	Listener(SourceId &source_id, socket::Address &source_address)
	    : m_source_id(source_id), m_source_address(source_address)
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

	/**
	 * Dump incoming position events as JSON to stdout.
	 */
	void on_position_sensor(
	    const Demarshaller::Header &header,
	    const Demarshaller::PositionSensor &position) override
	{
		const auto &ip = m_source_address;
		std::cout << json({{"source_name", header.source_name},
		                   {"source_hash", header.source_hash},
		                   {"ip", {ip.a, ip.b, ip.c, ip.d}},
		                   {"port", ip.port},
		                   {"seq", header.sequence},
		                   {"type", "position"},
		                   {"device", position.device_name},
		                   {"position", position.position}})
		          << std::endl;
	}

	void on_heartbeat(const Demarshaller::Header &header) override
	{
		const auto &ip = m_source_address;
		std::cout << json({{"source_name", header.source_name},
		                   {"source_hash", header.source_hash},
		                   {"ip", {ip.a, ip.b, ip.c, ip.d}},
		                   {"port", ip.port},
		                   {"seq", header.sequence},
		                   {"type", "heartbeat"}})
		          << std::endl;
	}
};

static void make_nonblock(int fd)
{
	int flags = err(fcntl(fd, F_GETFL));
	err(fcntl(fd, F_SETFL, flags | O_NONBLOCK));
}

int main(int argc, const char *argv[])
{
	int port;
	std::string device_name = "EV3_CLIENT";

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

	socket::Address source_address(0, 0, 0, 0, 0);
	socket::Address listen_address(0, 0, 0, 0, port);
	socket::Address target_address(0, 0, 0, 0, port);
	socket::UDP sock(listen_address);
	printf("Listening on %d.%d.%d.%d:%d as \"%s\"...\n", listen_address.a,
	       listen_address.b, listen_address.c, listen_address.d, port,
	       device_name.c_str());

	SourceId source_id(device_name.c_str());
	Marshaller marshaller(
	    [&](const uint8_t *buf, size_t buf_size) -> bool {
		    socket::Message msg(buf, buf_size);
		    sock.send(target_address, msg);
		    return true;
	    },
	    source_id.name(), source_id.hash());

	Demarshaller demarshaller;
	Listener listener(source_id, source_address);

	auto handle_sock = [&]() -> bool {
		socket::Message msg;
		if (!sock.recv(source_address, msg)) {
			return false;
		}
		demarshaller.parse(listener, msg.buf(), msg.size());
		return true;
	};

	size_t line_buf_size = 1024;
	char *line_buf = static_cast<char *>(malloc(line_buf_size));
	make_nonblock(STDIN_FILENO);
	auto handle_stdin = [&]() -> bool {
		try {
			// Read a new line from standard in
			ssize_t ret = getline(&line_buf, &line_buf_size, stdin);
			if (ret < 0 &&
			    (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
				return true;
			}
			else if (ret < 0) {
				return false;
			}

			// Try to parse the line as JSON
			json msg = json::parse(line_buf);

			// Parse the target address and port
			target_address.a = msg["ip"][0].get<int>();
			target_address.b = msg["ip"][1].get<int>();
			target_address.c = msg["ip"][2].get<int>();
			target_address.d = msg["ip"][3].get<int>();
			target_address.port = msg["port"].get<int>();

			// Read message-type dependent information and send the
			// corresponding message
			const std::string type = msg["type"].get<std::string>();
			if (type == "set_duty_cycle") {
				std::string device = msg["device"].get<std::string>();
				int duty_cycle = msg["duty_cycle"].get<int>();
				marshaller.write_set_duty_cycle(device.c_str(), duty_cycle);
			}
			else if (type == "reset") {
				marshaller.write_reset();
			}
		}
		catch (json::exception &e) {
			std::cout << json({{"type", "error"}, {"what", e.what()}})
			          << std::endl;
		}
		marshaller.flush();
		return true;
	};

	EventLoop()
	    .register_event(sock, handle_sock)
	    .register_event_fd(STDIN_FILENO, handle_stdin)
	    .run();

	return 0;
}

