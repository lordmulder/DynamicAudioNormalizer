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

#include "FrameBuffer.h"

#include "Common.h"

///////////////////////////////////////////////////////////////////////////////
// Frame Data
///////////////////////////////////////////////////////////////////////////////

FrameData::FrameData(const size_t &channels, const size_t &frameLength)
:
	m_channels(channels),
	m_frameLength(frameLength)
{
	m_data = new double*[m_channels];

	for(size_t c = 0; c < m_channels; c++)
	{
		m_data[c] = new double[m_frameLength];
	}

	clear();
}

FrameData::~FrameData(void)
{
	for(size_t c = 0; c < m_channels; c++)
	{
		MY_DELETE_ARRAY(m_data[c]);
	}

	MY_DELETE_ARRAY(m_data);
}

void FrameData::clear(void)
{
	for(size_t c = 0; c < m_channels; c++)
	{
		memset(m_data[c], 0, m_frameLength * sizeof(double));
	}
}

void FrameData::write(const double *const *const src, const size_t &srcOffset, const size_t &destOffset, const size_t &length)
{
	assert(length + destOffset <= m_frameLength);

	for(uint32_t c = 0; c < m_channels; c++)
	{
		memcpy(&m_data[c][destOffset], &src[c][srcOffset], length * sizeof(double));
	}
}

void FrameData::read(double **dest, const size_t &destOffset, const size_t &srcOffset, const size_t &length)
{
	assert(length + srcOffset <= m_frameLength);

	for(uint32_t c = 0; c < m_channels; c++)
	{
		memcpy(&dest[c][destOffset], &m_data[c][srcOffset], length * sizeof(double));
	}
}

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

FrameBuffer::FrameBuffer(const size_t &channels, const size_t &frameLength, const size_t &frameCount)
:
	m_channels(channels),
	m_frameLength(frameLength),
	m_frameCount(frameCount)
{
	m_framesFree = m_frameCount;
	m_framesUsed = 0;
	m_posPut = m_posGet = 0;

	m_frames = new FrameData*[m_frameCount];

	for(size_t i = 0; i < m_frameCount; i++)
	{
		m_frames[i] = new FrameData(m_channels, m_frameLength);
	}
}

FrameBuffer::~FrameBuffer(void)
{
	for(size_t i = 0; i < m_frameCount; i++)
	{
		MY_DELETE(m_frames[i]);
	}

	MY_DELETE_ARRAY(m_frames);
}

///////////////////////////////////////////////////////////////////////////////
// Reset
///////////////////////////////////////////////////////////////////////////////

void FrameBuffer::reset(void)
{
	m_framesFree = m_frameCount;
	m_framesUsed = 0;
	m_posPut = m_posGet = 0;

	for(size_t i = 0; i < m_frameCount; i++)
	{
		m_frames[i]->clear();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Put / Get Frame
///////////////////////////////////////////////////////////////////////////////

bool FrameBuffer::putFrame(const FrameData *src)
{
	if(m_framesFree < 1)
	{
		return false;
	}

	for(size_t c = 0; c < m_channels; c++)
	{
		memcpy(m_frames[m_posPut]->data(c), src->data(c), m_frameLength * sizeof(double));
	}

	m_posPut = ((m_posPut + 1) % m_frameCount);
	
	m_framesUsed++;
	m_framesFree--;

	return true;
}

bool FrameBuffer::getFrame(FrameData *dest)
{
	if(m_framesUsed < 1)
	{
		return false;
	}

	for(size_t c = 0; c < m_channels; c++)
	{
		memcpy(dest->data(c), m_frames[m_posGet]->data(c), m_frameLength * sizeof(double));
	}

	m_posGet = ((m_posGet + 1) % m_frameCount);
	
	m_framesUsed--;
	m_framesFree++;

	return true;
}
