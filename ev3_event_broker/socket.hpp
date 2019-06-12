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

#pragma once

#include <cstddef>
#include <cstdint>

namespace ev3_event_broker {

class Message {
private:
	const uint8_t *m_buf;
	size_t m_size;

public:
	Message() : m_buf(nullptr), m_size(0) {}

	Message(const uint8_t *buf, size_t size) : m_buf(buf), m_size(size) {}

	explicit operator bool() const { return m_buf && m_size; }

	const uint8_t *buf() const { return m_buf; }

	size_t size() const { return m_size; }
};

struct Address {
	uint8_t a, b, c, d;
	uint16_t port;

	Address() : a(0), b(0), c(0), d(0), port(0) {}

	Address(uint16_t port) : a(0), b(0), c(0), d(0), port(port) {}

	Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port = 0)
	    : a(a), b(b), c(c), d(d), port(port) {}
};

class UDPSocket {
private:
	static constexpr size_t BUF_SIZE = 4096;

	Address m_addr;
	int m_sockfd;
	uint8_t m_buf[BUF_SIZE];

public:
	UDPSocket(Address addr);
	~UDPSocket();

	bool recv(Address &addr, Message &msg);

	bool send(const Address &addr, const Message &msg);

	int fd() const { return m_sockfd; }
};

}  // namespace ev3_event_broker
