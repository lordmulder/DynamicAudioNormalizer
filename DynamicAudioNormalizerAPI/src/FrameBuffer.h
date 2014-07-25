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

#include <cassert>

class FrameData
{
public:
	FrameData(const size_t &channels, const size_t &frameLength);
	~FrameData(void);
	
	inline double *data(const size_t &channel)
	{
		assert(channel < m_channels);
		return m_data[channel];
	}
	
	inline const double *data(const size_t &channel) const
	{
		assert(channel < m_channels);
		return m_data[channel];
	}

	inline const size_t &channels(void)    { return m_channels;    }
	inline const size_t &frameLength(void) { return m_frameLength; }

	void write(const double *const *const src, const size_t &srcOffset, const size_t &destOffset, const size_t &length);
	void read(double **dest, const size_t &destOffset, const size_t &srcOffset, const size_t &length);

	void clear(void);
	
private:
	FrameData(const FrameData&) : m_channels(0), m_frameLength(0) { throw "unsupported"; }
	FrameData &operator=(const FrameData&)                        { throw "unsupported"; }

	const size_t m_channels;
	const size_t m_frameLength;
	
	double **m_data;
};

class FrameBuffer
{
public:
	FrameBuffer(const size_t &channels, const size_t &frameLength, const size_t &frameCount);
	~FrameBuffer(void);

	bool putFrame(const FrameData *src);
	bool getFrame(FrameData *dest);

	inline const size_t &channels(void)    { return m_channels;    }
	inline const size_t &frameLength(void) { return m_frameLength; }
	inline const size_t &frameCount(void)  { return m_frameCount;  }

	inline const size_t &framesFree(void)  { return m_framesFree;  }
	inline const size_t &framesUsed(void)  { return m_framesUsed;  }

	void reset(void);

private:
	FrameBuffer(const FrameBuffer&) : m_channels(0), m_frameLength(0), m_frameCount(0) { throw "unsupported"; }
	FrameBuffer &operator=(const FrameBuffer&)                                         { throw "unsupported"; }

	const size_t m_channels;
	const size_t m_frameLength;
	const size_t m_frameCount;

	size_t m_framesFree;
	size_t m_framesUsed;

	size_t m_posPut;
	size_t m_posGet;

	FrameData **m_frames;
};
