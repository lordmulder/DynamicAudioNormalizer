//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014-2019 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "FrameBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Frame Data
///////////////////////////////////////////////////////////////////////////////

DYNAUDNORM_NS::FrameData::FrameData(const uint32_t &channels, const uint32_t &frameLength)
:
	m_channels(channels),
	m_frameLength(frameLength)
{
	m_data = new double*[m_channels];

	for(uint32_t c = 0; c < m_channels; c++)
	{
		m_data[c] = new double[m_frameLength];
	}

	clear();
}

DYNAUDNORM_NS::FrameData::~FrameData(void)
{
	for(uint32_t c = 0; c < m_channels; c++)
	{
		MY_DELETE_ARRAY(m_data[c]);
	}

	MY_DELETE_ARRAY(m_data);
}

void DYNAUDNORM_NS::FrameData::clear(void)
{
	for(uint32_t c = 0; c < m_channels; c++)
	{
		memset(m_data[c], 0, m_frameLength * sizeof(double));
	}
}

///////////////////////////////////////////////////////////////////////////////
// Frame FIFO
///////////////////////////////////////////////////////////////////////////////

DYNAUDNORM_NS::FrameFIFO::FrameFIFO(const uint32_t &channels, const uint32_t &frameLength)
{
	m_data = new DYNAUDNORM_NS::FrameData(channels, frameLength);
	reset(false);
}

DYNAUDNORM_NS::FrameFIFO::~FrameFIFO(void)
{
	MY_DELETE(m_data);
}

void DYNAUDNORM_NS::FrameFIFO::reset(const bool &bForceClear)
{
	if(bForceClear) m_data->clear();
	m_posPut = m_posGet = m_leftGet = 0;
	m_leftPut = m_data->frameLength();
}

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

DYNAUDNORM_NS::FrameBuffer::FrameBuffer(const uint32_t &channels, const uint32_t &frameLength, const uint32_t &frameCount)
:
	m_channels(channels),
	m_frameLength(frameLength),
	m_frameCount(frameCount)
{
	m_framesFree = m_frameCount;
	m_framesUsed = 0;
	m_posPut = m_posGet = 0;

	m_frames = new DYNAUDNORM_NS::FrameData*[m_frameCount];

	for(uint32_t i = 0; i < m_frameCount; i++)
	{
		m_frames[i] = new DYNAUDNORM_NS::FrameData(m_channels, m_frameLength);
	}
}

DYNAUDNORM_NS::FrameBuffer::~FrameBuffer(void)
{
	for(uint32_t i = 0; i < m_frameCount; i++)
	{
		MY_DELETE(m_frames[i]);
	}

	MY_DELETE_ARRAY(m_frames);
}

///////////////////////////////////////////////////////////////////////////////
// Reset
///////////////////////////////////////////////////////////////////////////////

void DYNAUDNORM_NS::FrameBuffer::reset(void)
{
	m_framesFree = m_frameCount;
	m_framesUsed = 0;
	m_posPut = m_posGet = 0;

	for(uint32_t i = 0; i < m_frameCount; i++)
	{
		m_frames[i]->clear();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Put / Get Frame
///////////////////////////////////////////////////////////////////////////////

bool DYNAUDNORM_NS::FrameBuffer::putFrame(DYNAUDNORM_NS::FrameFIFO *const src)
{
	if((m_framesFree < 1) && (src->samplesLeftGet() < m_frameLength))
	{
		return false;
	}

	src->getSamples(m_frames[m_posPut], 0, m_frameLength);
	m_posPut = ((m_posPut + 1) % m_frameCount);
	
	m_framesUsed++;
	m_framesFree--;

	return true;
}

bool DYNAUDNORM_NS::FrameBuffer::getFrame(DYNAUDNORM_NS::FrameFIFO *const dest)
{
	if((m_framesUsed < 1) && (dest->samplesLeftPut() < m_frameLength))
	{
		return false;
	}

	dest->putSamples(m_frames[m_posGet], 0, m_frameLength);
	m_posGet = ((m_posGet + 1) % m_frameCount);
	
	m_framesUsed--;
	m_framesFree++;

	return true;
}
