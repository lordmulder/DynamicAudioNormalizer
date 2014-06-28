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

#include "Common.h"
#include "RingBuffer.h"

class Normalizer
{
public:
	Normalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec = 200, const bool channelsCoupled = true, const double aggressiveness = 0.1);
	~Normalizer(void);
	
	void processInplace(double **samplesIn, int64_t inputSize, int64_t &outputSize);

protected:
	void processNextFrame(void);
	double findPeakMagnitude(const uint32_t channel = UINT32_MAX);
	void updateAmplificationFactors(void);

private:
	const uint32_t m_channels;
	const uint32_t m_sampleRate;
	const uint32_t m_frameLen;

	const bool m_channelsCoupled;
	const double m_aggressiveness;

	double *m_amplificationFactor;

	RingBuffer **m_bufferSrc;
	RingBuffer **m_bufferOut;

	double **m_frameBuffer;
};
