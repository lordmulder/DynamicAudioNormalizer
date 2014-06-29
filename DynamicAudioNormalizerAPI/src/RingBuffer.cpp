///////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#include "RingBuffer.h"

#include "Common.h"

#include <cstring>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////

#define MOD(X) ((X) % this->m_capacity)

#ifdef _DEBUG
#undef FAST_RING_COPY
#else
#define FAST_RING_COPY 1
#endif

///////////////////////////////////////////////////////////////////////////////

RingBuffer::RingBuffer(const uint32_t capacity)
:
	m_capacity(capacity)
{
	m_buffer = new double[capacity];
	clear();
}

RingBuffer::~RingBuffer(void)
{
	MY_DELETE_ARRAY(m_buffer);
}

void RingBuffer::clear(void)
{
	memset(m_buffer, 0, sizeof(double) * m_capacity);
	m_freeCount = m_capacity;
	m_usedCount = m_posRd = m_posWr = 0;
}

void RingBuffer::read(double *data, const uint32_t count)
{
	if(m_usedCount < count)
	{
		throw std::runtime_error("Ring buffer cannot read that much -> out of data!");
	}

#ifdef FAST_RING_COPY
	if((m_posRd + count) <= m_capacity)
	{
		//Read operation can use a single memcpy
		memcpy(data, &m_buffer[m_posRd], sizeof(double) * count);
	}
	else
	{
		//Read range wrap around -> need two separate memcpy's!
		const uint32_t copyLen = m_capacity - m_posRd;
		memcpy(data, &m_buffer[m_posRd], sizeof(double) * copyLen);
		memcpy(&data[copyLen], m_buffer, sizeof(double) * (count - copyLen));
	}
#else
	for(uint32_t i = 0; i < count; i++)
	{
		data[i] = m_buffer[MOD(m_posRd+i)];
	}
#endif

	m_posRd = MOD(m_posRd + count);

	m_usedCount -= count;
	m_freeCount += count;
}

void RingBuffer::append(const double *data, const uint32_t count)
{
	if(m_freeCount < count)
	{
		throw std::runtime_error("Ring buffer cannot append that much -> out of memory!");
	}

#ifdef FAST_RING_COPY
	if((m_posWr + count) <= m_capacity)
	{
		//Write operation can use a single memcpy
		memcpy(&m_buffer[m_posWr], data, sizeof(double) * count);
	}
	else
	{
		//Write range wrap around -> need two separate memcpy's!
		const uint32_t copyLen = m_capacity - m_posWr;
		memcpy(&m_buffer[m_posWr], data, sizeof(double) * copyLen);
		memcpy(m_buffer, &data[copyLen], sizeof(double) * (count - copyLen));
	}
#else
	for(uint32_t i = 0; i < count; i++)
	{
		m_buffer[MOD(m_posWr+i)] = data[i];
	}
#endif

	m_posWr = MOD(m_posWr + count);

	m_usedCount += count;
	m_freeCount -= count;
}
