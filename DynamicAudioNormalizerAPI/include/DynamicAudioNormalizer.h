//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

//Standard Library include
#include <stdint.h>

//DLL Export Definitions
#ifdef _MSC_VER
#  ifdef MDYNAMICAUDIONORMALIZER_EXPORTS
#    pragma message("MDynamicAudioNormalizer DLL: Export")
#    define MDYNAMICAUDIONORMALIZER_DLL __declspec(dllexport)
#  else
#    ifndef MDYNAMICAUDIONORMALIZER_STATIC
#      pragma message("MDynamicAudioNormalizer DLL: Import")
#      define MDYNAMICAUDIONORMALIZER_DLL __declspec(dllimport)
#    else
#      define MDYNAMICAUDIONORMALIZER_DLL
#    endif
#  endif
#else
#  ifdef __GNUC__
#    ifdef MDYNAMICAUDIONORMALIZER_EXPORTS
#      define MDYNAMICAUDIONORMALIZER_DLL __attribute__ ((visibility ("default")))
#    else
#      define MDYNAMICAUDIONORMALIZER_DLL
#    endif
#  else
#    define MDYNAMICAUDIONORMALIZER_DLL
#  endif
#endif

//Utility macros
#define MDYNAMICAUDIONORMALIZER_GLUE1(X,Y) X##Y
#define MDYNAMICAUDIONORMALIZER_GLUE2(X,Y) MDYNAMICAUDIONORMALIZER_GLUE1(X,Y)

//Interface version
#define MDYNAMICAUDIONORMALIZER_CORE 4
#define MDYNAMICAUDIONORMALIZER_FUNCTION(X) MDYNAMICAUDIONORMALIZER_GLUE2(MDynamicAudioNormalizer_##X##_r, MDYNAMICAUDIONORMALIZER_CORE)
#define MDynamicAudioNormalizer MDYNAMICAUDIONORMALIZER_GLUE2(MDynamicAudioNormalizer_r,MDYNAMICAUDIONORMALIZER_CORE)

///////////////////////////////////////////////////////////////////////////////
// API for C++ application
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus /*C++ compiler!*/

//Standard Library include
#include <cstdio>

//Opaque Data Class
class MDynamicAudioNormalizer_PrivateData;

//Callback for log messages
typedef void (MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction))(const int logLevel, const char *const message);

	//Dynamic Normalizer Class
class MDYNAMICAUDIONORMALIZER_DLL MDynamicAudioNormalizer
{
public:
	//Constructor & Destructor
	MDynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec = 500, const uint32_t filterSize = 31, const double peakValue = 0.95, const double maxAmplification = 10.0, const double targetRms = 0.0, const bool channelsCoupled = true, const bool enableDCCorrection = false, const bool altBoundaryMode = false, FILE *const logFile = NULL);
	virtual ~MDynamicAudioNormalizer(void);
	
	//Public API
	bool initialize(void);
	bool processInplace(double **samplesInOut, const int64_t inputSize, int64_t &outputSize);
	bool flushBuffer(double **samplesOut, const int64_t bufferSize, int64_t &outputSize);
	bool reset(void);

	//Type finfitions
	typedef MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction) LogFunction;
	enum { LOG_LEVEL_NFO = 0, LOG_LEVEL_WRN = 1, LOG_LEVEL_ERR = 2 };

	//Static functions
	static void getVersionInfo(uint32_t &major, uint32_t &minor,uint32_t &patch);
	static void getBuildInfo(const char **date, const char **time, const char **compiler, const char **arch, bool &debug);
	static LogFunction *setLogFunction(LogFunction *const logFunction);

private:
	MDynamicAudioNormalizer(const MDynamicAudioNormalizer&) : p(NULL)  { throw "unsupported"; }
	MDynamicAudioNormalizer &operator=(const MDynamicAudioNormalizer&) { throw "unsupported"; }

	MDynamicAudioNormalizer_PrivateData *const p;
};

#endif //__cplusplus

///////////////////////////////////////////////////////////////////////////////
// API for C applications
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//Standard Library include
#include <stdio.h>

//Opaque handle
typedef struct MDynamicAudioNormalizer_Handle MDynamicAudioNormalizer_Handle;

//Global Functions
MDYNAMICAUDIONORMALIZER_DLL MDynamicAudioNormalizer_Handle* MDYNAMICAUDIONORMALIZER_FUNCTION(createInstance)(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const int channelsCoupled, const int enableDCCorrection, const int altBoundaryMode, FILE *const logFile);
MDYNAMICAUDIONORMALIZER_DLL void MDYNAMICAUDIONORMALIZER_FUNCTION(destroyInstance)(MDynamicAudioNormalizer_Handle **handle);
MDYNAMICAUDIONORMALIZER_DLL int  MDYNAMICAUDIONORMALIZER_FUNCTION(processInplace)(MDynamicAudioNormalizer_Handle *handle, double **samplesInOut, int64_t inputSize, int64_t *outputSize);
MDYNAMICAUDIONORMALIZER_DLL int  MDYNAMICAUDIONORMALIZER_FUNCTION(flushBuffer)(MDynamicAudioNormalizer_Handle *handle, double **samplesOut, const int64_t bufferSize, int64_t *outputSize);
MDYNAMICAUDIONORMALIZER_DLL int  MDYNAMICAUDIONORMALIZER_FUNCTION(reset)(MDynamicAudioNormalizer_Handle *handle);
MDYNAMICAUDIONORMALIZER_DLL void MDYNAMICAUDIONORMALIZER_FUNCTION(getVersionInfo)(uint32_t *major, uint32_t *minor,uint32_t *patch);
MDYNAMICAUDIONORMALIZER_DLL void MDYNAMICAUDIONORMALIZER_FUNCTION(getBuildInfo)(const char **date, const char **time, const char **compiler, const char **arch, int *debug);
MDYNAMICAUDIONORMALIZER_DLL MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction) *MDYNAMICAUDIONORMALIZER_FUNCTION(setLogFunction)(MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction) *const logFunction);

#ifdef __cplusplus
}
#endif //__cplusplus
