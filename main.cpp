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

class UDPSocketServer {
private:
	static constexpr size_t BUF_SIZE = 4096;

	int m_sockfd;
	int m_port;
	uint8_t m_buf[BUF_SIZE];

	struct sockaddr_in m_serveraddr;
	struct sockaddr_in m_clientaddr;

public:
	UDPSocketServer(int port) : m_sockfd(-1), m_port(port) {
		// Create the socket
		ERR(m_sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0));

		// Mark the socket as reusable
		int optval;
		ERR(setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR,
		               (const void *)&optval, sizeof(int)));

		// Bind to an address
		bzero((char *)&m_serveraddr, sizeof(m_serveraddr));
		m_serveraddr.sin_family = AF_INET;
		m_serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		m_serveraddr.sin_port = htons((unsigned short)m_port);

		ERR(bind(m_sockfd,
		         reinterpret_cast<const struct sockaddr *>(&m_serveraddr),
		         sizeof(m_serveraddr)));
	}

	Message recv() {
		while (true) {
			socklen_t addrlen = sizeof(m_clientaddr);
			ssize_t count = recvfrom(
			    m_sockfd, &m_buf, BUF_SIZE, 0,
			    reinterpret_cast<struct sockaddr *>(&m_clientaddr), &addrlen);
			if (count == 0) {
				return Message();  // Socket has been shut down
			} else if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				continue;  // Try again
			} else if (count < 0) {
				throw std::system_error(errno, std::system_category());
			} else {
				return Message(m_buf, count);
			}
		}
	}

	~UDPSocketServer() {
		if (m_sockfd >= 0) {
			close(m_sockfd);
		}
	}
};

int main(int argc, char *argv[]) {
	UDPSocketServer srv(4721);
	Message msg;
	while ((msg = srv.recv())) {
		write(STDOUT_FILENO, msg.buf(), msg.size());
	}
}
