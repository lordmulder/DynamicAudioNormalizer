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

class DynamicNormalizer
{
public:
	DynamicNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, FILE *const logFile = NULL);
	~DynamicNormalizer(void);
	
	enum
	{
		PASS_1ST = 0,
		PASS_2ND = 1
	}
	pass_t;

	bool processInplace(double **samplesIn, int64_t inputSize, int64_t &outputSize);
	bool setPass(const int pass);

protected:
	void processNextFrame(void);
	void analyzeCurrentFrame(void);
	void amplifyCurrentFrame(void);
	
	double findPeakMagnitude(const uint32_t channel = UINT32_MAX);
	void writeToLogFile(void);

private:
	class PrivateData;
	PrivateData *const p;

	const uint32_t m_channels;
	const uint32_t m_sampleRate;
	const uint32_t m_frameLen;

	const bool m_channelsCoupled;
	const bool m_enableDCCorrection;
	const double m_peakValue;
	const double m_maxAmplification;

	FILE *const m_logFile;
};
