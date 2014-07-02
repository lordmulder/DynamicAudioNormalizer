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

//Standard C++ includes
#include <stdint.h>
#include <cstdio>

//DLL Export Definitions
#ifdef _MSC_VER
#  ifdef MDYNAMICAUDIONORMALIZER_EXPORTS
#    pragma message("MDynamicAudioNormalizer DLL: Export")
#    define MDYNAMICAUDIONORMALIZER_DLL __declspec(dllexport)
#  else
#    pragma message("MDynamicAudioNormalizer DLL: Import")
#    define MDYNAMICAUDIONORMALIZER_DLL __declspec(dllimport)
#  endif
#else
#  ifdef __GNUG__
#    define MDYNAMICAUDIONORMALIZER_DLL __attribute__ ((visibility ("default")))
#  else
#    define MDYNAMICAUDIONORMALIZER_DLL
#  endif
#endif

//Utility macros
#define MDYNAMICAUDIONORMALIZER_GLUE1(X,Y) X##Y
#define MDYNAMICAUDIONORMALIZER_GLUE2(X,Y) MDYNAMICAUDIONORMALIZER_GLUE1(X,Y)

//Interface version
#define MDYNAMICAUDIONORMALIZER_CORE 1
#define MDynamicAudioNormalizer MDYNAMICAUDIONORMALIZER_GLUE2(MDynamicAudioNormalizer_r,MDYNAMICAUDIONORMALIZER_CORE)

//Opaque Data Class
class MDynamicAudioNormalizer_PrivateData;

//Dynamic Normalizer Class
class MDYNAMICAUDIONORMALIZER_DLL MDynamicAudioNormalizer
{
public:
	//Constant
	enum { PASS_1ST = 0, PASS_2ND = 1 } pass_t;

	//Constructor & Destructor
	MDynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose = false, FILE *const logFile = NULL);
	virtual ~MDynamicAudioNormalizer(void);
	
	//Public API
	bool initialize(void);
	bool processInplace(double **samplesInOut, int64_t inputSize, int64_t &outputSize);
	bool setPass(const int pass);

	//Static functions
	static void getVersionInfo(uint32_t &major, uint32_t &minor,uint32_t &patch);
	static void getBuildInfo(const char **date, const char **time, const char **compiler, const char **arch, bool &debug);

private:
	MDynamicAudioNormalizer_PrivateData *const p;
};
