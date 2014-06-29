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

#include <stdint.h>
#include <cstdio>

//Opaque Data Class
class DynamicNormalizer_PrivateData;

//Dynamic Normalizer Class
class DynamicNormalizer
{
public:
	//Constant
	enum { PASS_1ST = 0, PASS_2ND = 1 } pass_t;

	//Constructor & Destructor
	DynamicNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, FILE *const logFile = NULL);
	virtual ~DynamicNormalizer(void);
	
	//Public API
	bool processInplace(double **samplesIn, int64_t inputSize, int64_t &outputSize);
	bool setPass(const int pass);

private:
	DynamicNormalizer_PrivateData *const p;
};
