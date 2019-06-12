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

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <system_error>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ev3_event_broker/error.hpp>
#include <ev3_event_broker/socket.hpp>

namespace ev3_event_broker {

/******************************************************************************
 * Helper functions                                                           *
 ******************************************************************************/

void addr_to_sockaddr(const Address &addr, struct sockaddr_in *sockaddr) {
	bzero(reinterpret_cast<char*>(sockaddr), sizeof(*sockaddr));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr =
	    htonl((addr.a << 24) | (addr.b << 16) | (addr.c << 8) | (addr.d << 0));
	sockaddr->sin_port = htons(addr.port);
}

Address addr_from_sockaddr(const struct sockaddr_in *addr) {
	uint32_t s_addr = ntohl(addr->sin_addr.s_addr);

	uint8_t a = uint8_t((s_addr >> 24) & 0xFF);
	uint8_t b = uint8_t((s_addr >> 16) & 0xFF);
	uint8_t c = uint8_t((s_addr >> 8) & 0xFF);
	uint8_t d = uint8_t((s_addr >> 0) & 0xFF);
	uint16_t port = ntohs(addr->sin_port);

	return Address(a, b, c, d, port);
}

/******************************************************************************
 * UDPSocket Implementation                                                   *
 ******************************************************************************/

UDPSocket::UDPSocket(Address addr) : m_sockfd(-1), m_addr(addr) {
	int optval;

	// Create the socket
	m_sockfd = err(socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0));

	// Mark the socket as reusable
	optval = 1;
	err(setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR,
	               static_cast<const void *>(&optval), sizeof(optval)));

	// Enable broadcasting
	optval = 1;
	err(setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST,
	               static_cast<const void *>(&optval), sizeof(optval)));

	// Bind to an address
	struct sockaddr_in serveraddr;
	addr_to_sockaddr(addr, &serveraddr);
	err(bind(m_sockfd, reinterpret_cast<const struct sockaddr *>(&serveraddr),
	         sizeof(serveraddr)));
}

bool UDPSocket::recv(Address &addr, Message &msg) {
	while (true) {
		struct sockaddr_in clientaddr;
		socklen_t addrlen = sizeof(clientaddr);
		ssize_t count = recvfrom(
		    m_sockfd, &m_buf, BUF_SIZE, 0,
		    reinterpret_cast<struct sockaddr *>(&clientaddr), &addrlen);
		if (count == 0) {
			return false;  // Socket has been shut down
		} else if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			continue;  // Try again
		} else if (count < 0) {
			throw std::system_error(errno, std::system_category());
		} else {
			msg = Message(m_buf, count);
			addr = addr_from_sockaddr(&clientaddr);
			return true;
		}
	}
}

bool UDPSocket::send(const Address &addr, const Message &msg) {
	while (true) {
		struct sockaddr_in clientaddr;
		addr_to_sockaddr(addr, &clientaddr);
		ssize_t count = sendto(m_sockfd, msg.buf(), msg.size(), 0,
		                       reinterpret_cast<struct sockaddr *>(&clientaddr),
		                       sizeof(clientaddr));
		if (count == msg.size()) {
			return true;
		} else if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			continue;
		} else if (count < 0) {
			throw std::system_error(errno, std::system_category());
		} else {
			return false;
		}
	}
}

UDPSocket::~UDPSocket() {
	if (m_sockfd >= 0) {
		close(m_sockfd);
	}
}

}
