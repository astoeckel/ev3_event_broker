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

#include <cstddef>
#include <cstdint>
#include <functional>

namespace ev3_event_broker {

static constexpr uint32_t SYNC = 0xCAA29C3AU;
static constexpr uint8_t TYPE_POSITION_SENSOR = 0x01;
static constexpr size_t MARSHALLER_BUF_SIZE = 1280;
static constexpr size_t N_SOURCE_NAME_CHARS = 8;
static constexpr size_t N_SOURCE_HASH_CHARS = 8;
static constexpr size_t N_DEVICE_NAME_CHARS = 16;
static constexpr size_t HEADER_SIZE = 4 + N_SOURCE_NAME_CHARS + N_SOURCE_HASH_CHARS + 1;
static constexpr size_t POSITION_SENSOR_SIZE = 1 + N_DEVICE_NAME_CHARS + 4;

class Marshaller {
public:
	using Callback = std::function<bool(const uint8_t *buf, size_t size)>;

private:
	Callback m_cback;
	uint8_t m_buf[MARSHALLER_BUF_SIZE];
	size_t m_buf_ptr;
	uint8_t *m_message_count;
	bool m_good;

	void flush_if_no_space(size_t size_required);

public:
	Marshaller(const Callback &cback, const char *source_name,
	           const char *source_hash);

	explicit operator bool() const { return m_good; }

	Marshaller &flush();

	Marshaller &write_position_sensor(const char *device_name, int position);
};

class Demarshaller {
public:
	struct Header {
		char source_name[N_SOURCE_NAME_CHARS];
		char source_hash[N_SOURCE_HASH_CHARS];
		uint8_t n_messages;
	};

	struct PositionSensor {
		char device_name[N_DEVICE_NAME_CHARS];
		int32_t position;
	};

	struct Listener {
		Listener(){};

		virtual ~Listener(){};

		virtual void on_position_sensor(const Header &,
		                                const PositionSensor &){};
	};

private:
	uint32_t m_sync;
	Header m_header;
	uint8_t m_type;
	PositionSensor m_position_sensor;

	Listener &m_listener;

public:
	Demarshaller(Listener &listener);

	void parse(const uint8_t *buf, size_t buf_size);
};

}  // namespace ev3_event_broker
