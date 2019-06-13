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

#include <ev3_event_broker/marshaller.hpp>

namespace ev3_event_broker {

/******************************************************************************
 * Helper functions                                                           *
 ******************************************************************************/

static inline uint8_t *write_fixed_size_string(const char *src, uint8_t *tar,
                                        size_t tar_size)
{
	size_t i = 0;
	for (; (i < tar_size) && src[i]; i++) {
		tar[i] = src[i];
	}
	for (; (i < tar_size); i++) {
		tar[i] = 0;
	}
	return tar + tar_size;
}

static inline uint8_t *write_uint32(uint32_t value, uint8_t *tar)
{
	*(tar++) = (value >> 24) & 0xFF;
	*(tar++) = (value >> 16) & 0xFF;
	*(tar++) = (value >> 8) & 0xFF;
	*(tar++) = (value >> 0) & 0xFF;
	return tar;
}

static inline uint8_t *write_int32(int32_t value, uint8_t *tar)
{
	return write_uint32(static_cast<uint32_t>(value), tar);
}

static inline uint8_t *write_uint16(uint16_t value, uint8_t *tar)
{
	*(tar++) = (value >> 8) & 0xFF;
	*(tar++) = (value >> 0) & 0xFF;
	return tar;
}

static inline uint8_t *write_int16(int16_t value, uint8_t *tar)
{
	return write_uint16(static_cast<uint16_t>(value), tar);
}

static inline uint8_t *write_uint8(uint8_t value, uint8_t *tar)
{
	*(tar++) = (value >> 0) & 0xFF;
	return tar;
}

static inline uint8_t *write_int8(int8_t value, uint8_t *tar)
{
	return write_uint8(static_cast<uint8_t>(value), tar);
}

/******************************************************************************
 * Class Marshaller                                                           *
 ******************************************************************************/

Marshaller::Marshaller(const Marshaller::Callback &cback,
                       const char *source_name, const char *source_hash)
    : m_cback(cback), m_good(true)
{
	// Write the header to the internal buffer, remember the location of the
	// message count
	uint8_t *tar = m_buf;
	tar = write_uint32(SYNC, tar);
	tar = write_fixed_size_string(source_name, tar, N_SOURCE_NAME_CHARS);
	tar = write_fixed_size_string(source_hash, tar, N_SOURCE_HASH_CHARS);
	m_message_count = tar++;
	*m_message_count = 0;
	m_buf_ptr = tar - m_buf;
}

void Marshaller::flush_if_no_space(size_t size_required)
{
	if (m_buf_ptr + size_required > sizeof(m_buf)) {
		flush();
	}
}

Marshaller &Marshaller::flush()
{
	if (m_good && *m_message_count > 0) {
		m_good = m_good && m_cback(m_buf, m_buf_ptr);
	}
	*m_message_count = 0;
	m_buf_ptr = m_message_count - m_buf + 1;

	return *this;
}

Marshaller &Marshaller::write_position_sensor(const char *device_name,
                                              int32_t position)
{
	flush_if_no_space(POSITION_SENSOR_SIZE);

	uint8_t *tar = m_buf + m_buf_ptr;
	tar = write_uint8(TYPE_POSITION_SENSOR, tar);
	tar = write_fixed_size_string(device_name, tar, N_DEVICE_NAME_CHARS);
	tar = write_int32(position, tar);
	m_buf_ptr = tar - m_buf;
	(*m_message_count)++;

	return *this;
}

}  // namespace ev3_event_broker
