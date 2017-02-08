//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cassert>
#include <cstring>
#include <stdint.h>

class MDynamicAudioNormalizer_FrameData
{
public:
	MDynamicAudioNormalizer_FrameData(const uint32_t &channels, const uint32_t &frameLength);
	~MDynamicAudioNormalizer_FrameData(void);
	
	inline double *data(const uint32_t &channel)
	{
		assert(channel < m_channels);
		return m_data[channel];
	}
	
	inline const double *data(const uint32_t &channel) const
	{
		assert(channel < m_channels);
		return m_data[channel];
	}

	inline const uint32_t &channels(void)    { return m_channels;    }
	inline const uint32_t &frameLength(void) { return m_frameLength; }

	inline void write(const double *const *const src, const uint32_t &srcOffset, const uint32_t &destOffset, const uint32_t &length)
	{
		assert(length + destOffset <= m_frameLength);
		for(uint32_t c = 0; c < m_channels; c++)
		{
			memcpy(&m_data[c][destOffset], &src[c][srcOffset], length * sizeof(double));
		}
	}

	inline void write(const MDynamicAudioNormalizer_FrameData *const src, const uint32_t &srcOffset, const uint32_t &destOffset, const uint32_t &length)
	{
		assert(length + destOffset <= m_frameLength);
		for(uint32_t c = 0; c < m_channels; c++)
		{
			memcpy(&m_data[c][destOffset], &src->data(c)[srcOffset], length * sizeof(double));
		}
	}

	inline void read(double *const *const dest, const uint32_t &destOffset, const uint32_t &srcOffset, const uint32_t &length)
	{
		assert(length + srcOffset <= m_frameLength);
		for(uint32_t c = 0; c < m_channels; c++)
		{
			memcpy(&dest[c][destOffset], &m_data[c][srcOffset], length * sizeof(double));
		}
	}

	inline void read(MDynamicAudioNormalizer_FrameData *const dest, const uint32_t &destOffset, const uint32_t &srcOffset, const uint32_t &length)
	{
		assert(length + srcOffset <= m_frameLength);
		for(uint32_t c = 0; c < m_channels; c++)
		{
			memcpy(&dest->data(c)[destOffset], &m_data[c][srcOffset], length * sizeof(double));
		}
	}

	void clear(void);
	
private:
	MDynamicAudioNormalizer_FrameData(const MDynamicAudioNormalizer_FrameData&) : m_channels(0), m_frameLength(0) { throw "unsupported"; }
	MDynamicAudioNormalizer_FrameData &operator=(const MDynamicAudioNormalizer_FrameData&)                        { throw "unsupported"; }

	const uint32_t m_channels;
	const uint32_t m_frameLength;
	
	double **m_data;
};

class MDynamicAudioNormalizer_FrameFIFO
{
public:
	MDynamicAudioNormalizer_FrameFIFO(const uint32_t &channels, const uint32_t &frameLength);
	~MDynamicAudioNormalizer_FrameFIFO(void);

	inline uint32_t samplesLeftPut(void) { return m_leftPut; }
	inline uint32_t samplesLeftGet(void) { return m_leftGet; }

	inline void putSamples(const double *const *const src, const uint32_t &srcOffset,const uint32_t &length)
	{
		assert(length <= samplesLeftPut());
		m_data->write(src, srcOffset, m_posPut, length);
		m_posPut  += length;
		m_leftPut -= length;
		m_leftGet += length;
	}

	inline void putSamples(const MDynamicAudioNormalizer_FrameData *const src, const uint32_t &srcOffset,const uint32_t &length)
	{
		assert(length <= samplesLeftPut());
		m_data->write(src, srcOffset, m_posPut, length);
		m_posPut  += length;
		m_leftPut -= length;
		m_leftGet += length;
	}

	inline void getSamples(double *const *const dest, const uint32_t &destOffset, const uint32_t &length)
	{
		assert(length <= samplesLeftGet());
		m_data->read(dest, destOffset, m_posGet, length);
		m_posGet  += length;
		m_leftGet -= length;
	}
	
	inline void getSamples(MDynamicAudioNormalizer_FrameData *const dest, const uint32_t &destOffset, const uint32_t &length)
	{
		assert(length <= samplesLeftGet());
		m_data->read(dest, destOffset, m_posGet, length);
		m_posGet  += length;
		m_leftGet -= length;
	}

	inline MDynamicAudioNormalizer_FrameData *data(void)
	{
		return m_data;
	}

	void reset(const bool &bForceClear = true);

private:
	MDynamicAudioNormalizer_FrameData *m_data;

	uint32_t m_posPut;
	uint32_t m_posGet;
	uint32_t m_leftPut;
	uint32_t m_leftGet;
};

class MDynamicAudioNormalizer_FrameBuffer
{
public:
	MDynamicAudioNormalizer_FrameBuffer(const uint32_t &channels, const uint32_t &frameLength, const uint32_t &frameCount);
	~MDynamicAudioNormalizer_FrameBuffer(void);

	bool putFrame(MDynamicAudioNormalizer_FrameFIFO *const src);
	bool getFrame(MDynamicAudioNormalizer_FrameFIFO *const dest);

	inline const uint32_t &channels(void)    { return m_channels;    }
	inline const uint32_t &frameLength(void) { return m_frameLength; }
	inline const uint32_t &frameCount(void)  { return m_frameCount;  }

	inline const uint32_t &framesFree(void)  { return m_framesFree;  }
	inline const uint32_t &framesUsed(void)  { return m_framesUsed;  }

	void reset(void);

private:
	MDynamicAudioNormalizer_FrameBuffer(const MDynamicAudioNormalizer_FrameBuffer&) : m_channels(0), m_frameLength(0), m_frameCount(0) { throw "unsupported"; }
	MDynamicAudioNormalizer_FrameBuffer &operator=(const MDynamicAudioNormalizer_FrameBuffer&)                                         { throw "unsupported"; }

	const uint32_t m_channels;
	const uint32_t m_frameLength;
	const uint32_t m_frameCount;

	uint32_t m_framesFree;
	uint32_t m_framesUsed;

	uint32_t m_posPut;
	uint32_t m_posGet;

	MDynamicAudioNormalizer_FrameData **m_frames;
};
