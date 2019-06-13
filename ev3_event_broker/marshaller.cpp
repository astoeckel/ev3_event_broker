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

#include <type_traits>

#include <ev3_event_broker/marshaller.hpp>

namespace ev3_event_broker {

/******************************************************************************
 * Helper functions                                                           *
 ******************************************************************************/

static inline uint8_t *write_fixed_size_string(const char *src, uint8_t *tar,
                                               size_t tar_size) {
	size_t i = 0;
	for (; (i < tar_size) && src[i]; i++) {
		tar[i] = src[i];
	}
	for (; (i < tar_size); i++) {
		tar[i] = 0;
	}
	return tar + tar_size;
}

template <typename T, typename S = typename std::make_unsigned<T>::type>
static inline uint8_t *write_int(T value, uint8_t *tar) {
	S value_unsigned = static_cast<S>(value);
	for (size_t i = 0; i < sizeof(T); i++) {
		*(tar++) = (value_unsigned >> (8 * (sizeof(T) - (i + 1)))) & 0xFF;
	}
	return tar;
}

static inline const uint8_t *read_fixed_size_string(char *tar,
                                                    const uint8_t *src,
                                                    uint8_t size) {
	for (size_t i = 0; i < size; i++) {
		tar[i] = src[i];
	}
	return src + size;
}

template <typename T, typename S = typename std::make_unsigned<T>::type>
static inline const uint8_t *read_int(T *value, uint8_t const *src) {
	S value_unsigned = 0;
	for (size_t i = 0; i < sizeof(T); i++) {
		value_unsigned |= static_cast<S>(*(src++))
		                  << (8 * (sizeof(T) - (i + 1)));
	}
	*value = static_cast<T>(value_unsigned);
	return src;
}

/******************************************************************************
 * Class Marshaller                                                           *
 ******************************************************************************/

Marshaller::Marshaller(const Marshaller::Callback &cback,
                       const char *source_name, const char *source_hash)
    : m_cback(cback), m_good(true) {
	// Write the header to the internal buffer, remember the location of the
	// message count
	uint8_t *tar = m_buf;
	tar = write_int<uint32_t>(SYNC, tar);
	tar = write_fixed_size_string(source_name, tar, N_SOURCE_NAME_CHARS);
	tar = write_fixed_size_string(source_hash, tar, N_SOURCE_HASH_CHARS);

	m_sequence_offs = tar - m_buf;
	tar += sizeof(m_sequence) + sizeof(m_message_count);

	m_header_offs = tar - m_buf;
	m_buf_ptr = m_header_offs;
}

void Marshaller::flush_if_no_space(size_t size_required) {
	if (m_buf_ptr + size_required > sizeof(m_buf)) {
		flush();
	}
}

Marshaller &Marshaller::flush() {
	if (m_good && m_message_count > 0) {
		uint8_t *tar = m_buf + m_sequence_offs;
		tar = write_int<uint32_t>(m_sequence, tar);
		tar = write_int<uint8_t>(m_message_count, tar);
		m_good = m_good && m_cback(m_buf, m_buf_ptr);
	}
	m_message_count = 0;
	m_sequence++;
	m_buf_ptr = m_header_offs;

	return *this;
}

Marshaller &Marshaller::write_position_sensor(const char *device_name,
                                              int32_t position) {
	flush_if_no_space(POSITION_SENSOR_SIZE);

	uint8_t *tar = m_buf + m_buf_ptr;
	tar = write_int<uint8_t>(TYPE_POSITION_SENSOR, tar);
	tar = write_fixed_size_string(device_name, tar, N_DEVICE_NAME_CHARS);
	tar = write_int<int32_t>(position, tar);
	m_buf_ptr = tar - m_buf;
	m_message_count++;

	return *this;
}

/******************************************************************************
 * Class Demarshaller                                                         *
 ******************************************************************************/

Demarshaller::Demarshaller() : m_sync(0) {}

void Demarshaller::parse(Listener &listener, const uint8_t *buf,
                         size_t buf_size) {
	const uint8_t *src_end = buf + buf_size;
	uint8_t const *src = buf;
	while (src < src_end) {
		// Synchhronize with the sync word
		if (m_sync != SYNC) {
			m_sync = (m_sync << 8) | (*src++);
			continue;
		}

		// Read the message header
		if (size_t(src_end - src) < HEADER_SIZE) {
			return;
		}
		src = read_fixed_size_string(m_header.source_name, src,
		                             N_SOURCE_NAME_CHARS);
		src = read_fixed_size_string(m_header.source_hash, src,
		                             N_SOURCE_NAME_CHARS);
		src = read_int<uint32_t>(&m_header.sequence, src);
		src = read_int<uint8_t>(&m_header.n_messages, src);

		// Read the individual messages
		for (size_t i = 0; i < m_header.n_messages; i++) {
			// Make sure we can read the message type
			if (src >= src_end) {
				return;
			}
			src = read_int<uint8_t>(&m_type, src);

			// Parse the individual messages
			switch (m_type) {
				case TYPE_POSITION_SENSOR:
					src = read_fixed_size_string(m_position_sensor.device_name,
					                             src, N_DEVICE_NAME_CHARS);
					src = read_int<int32_t>(&m_position_sensor.position, src);
					listener.on_position_sensor(m_header, m_position_sensor);
					break;
				default:
					return;
			}
		}

		// Reset the sync word
		m_sync = 0;
	}
}

}  // namespace ev3_event_broker
