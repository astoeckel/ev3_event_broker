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
#include <cstdlib>
#include <cstring>
#include <system_error>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(X)                                                      \
	do {                                                            \
		if ((X) < 0) {                                              \
			throw std::system_error(errno, std::system_category()); \
		}                                                           \
	} while (false)

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

class Address {
private:
	uint8_t m_a, m_b, m_c, m_d;
	uint16_t m_port;

public:
	Address() : m_a(0), m_b(0), m_c(0), m_d(0), m_port(0) {}

	Address(uint16_t port) : m_a(0), m_b(0), m_c(0), m_d(0), m_port(port) {}

	Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port = 0)
	    : m_a(a), m_b(b), m_c(c), m_d(d), m_port(port) {}

	void to_sockaddr(struct sockaddr_in *addr) const {
		bzero((char *)addr, sizeof(*addr));
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr =
		    htonl((m_a << 24) | (m_b << 16) | (m_c << 8) | (m_d << 0));
		addr->sin_port = htons((unsigned short)m_port);
	}

	Address& from_sockaddr(const struct sockaddr_in *addr) {
		uint32_t s_addr = ntohl(addr->sin_addr.s_addr);
		m_a = uint8_t((s_addr >> 24) & 0xFF);
		m_b = uint8_t((s_addr >> 16) & 0xFF);
		m_c = uint8_t((s_addr >> 8) & 0xFF);
		m_d = uint8_t((s_addr >> 0) & 0xFF);
		m_port = ntohs(addr->sin_port);
		return *this;
	}
};

class UDPSocket {
private:
	static constexpr size_t BUF_SIZE = 4096;

	Address m_addr;
	int m_sockfd;
	uint8_t m_buf[BUF_SIZE];

public:
	UDPSocket(Address addr) : m_sockfd(-1), m_addr(addr) {
		int optval;

		// Create the socket
		ERR(m_sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0));

		// Mark the socket as reusable
		optval = 1;
		ERR(setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR,
		               static_cast<const void *>(&optval), sizeof(optval)));

		// Enable broadcasting
		optval = 1;
		ERR(setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST,
		               static_cast<const void *>(&optval), sizeof(optval)));

		// Bind to an address
		struct sockaddr_in serveraddr;
		m_addr.to_sockaddr(&serveraddr);
		ERR(bind(m_sockfd,
		         reinterpret_cast<const struct sockaddr *>(&serveraddr),
		         sizeof(serveraddr)));
	}

	bool recv(Address &addr, Message &msg) {
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
				addr.from_sockaddr(&clientaddr);
				return true;
			}
		}
	}

	bool send(const Address &addr, const Message &msg) {
		while (true) {
			struct sockaddr_in clientaddr;
			addr.to_sockaddr(&clientaddr);
			ssize_t count =
			    sendto(m_sockfd, msg.buf(), msg.size(), 0,
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

	~UDPSocket() {
		if (m_sockfd >= 0) {
			close(m_sockfd);
		}
	}

	int fd() const { return m_sockfd; }
};

int main(int argc, char *argv[]) {
	UDPSocket sock(Address(0, 0, 0, 0, 4721));
	Address addr;
	Message msg;
	while (sock.recv(addr, msg)) {
		write(STDOUT_FILENO, msg.buf(), msg.size());
		sock.send(addr, msg);
	}
}
