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

#pragma once

#include <cstdlib>
#include <stdint.h>

class RingBuffer
{
public:
	RingBuffer(const uint32_t capacity);
	~RingBuffer(void);

	void read(double *data, const uint32_t count);
	void append(const double *data, const uint32_t count);
		
	inline uint32_t getUsed(void) const { return m_usedCount; }
	inline uint32_t getFree(void) const { return m_freeCount; }
		
	void clear(void);

private:
	RingBuffer &operator=(const RingBuffer&) { throw 666; }

	double *m_buffer;
	const size_t m_capacity;
		
	uint32_t m_freeCount;
	uint32_t m_usedCount;
		
	uint32_t m_posRd;
	uint32_t m_posWr;
};
