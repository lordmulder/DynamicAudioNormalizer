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

#include "DynamicAudioNormalizer.h"

#define INT2BOOL(X) ((X) != 0)

extern "C"
{
	MDynamicAudioNormalizer_Handle* MDYNAMICAUDIONORMALIZER_FUNCTION(createInstance)(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const double compressFactor, const int channelsCoupled, const int enableDCCorrection, const int altBoundaryMode, FILE *const logFile)
	{
		try
		{
			MDynamicAudioNormalizer *const instance = new MDynamicAudioNormalizer(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, INT2BOOL(channelsCoupled), INT2BOOL(enableDCCorrection), INT2BOOL(altBoundaryMode), logFile);
			if(instance->initialize())
			{
				return reinterpret_cast<MDynamicAudioNormalizer_Handle*>(instance);
			}
			else
			{
				delete instance;
				return NULL;
			}
		}
		catch(...)
		{
			return NULL;
		}
	}

	MDYNAMICAUDIONORMALIZER_DLL void MDYNAMICAUDIONORMALIZER_FUNCTION(destroyInstance)(MDynamicAudioNormalizer_Handle **const handle)
	{
		if(MDynamicAudioNormalizer **const instance = reinterpret_cast<MDynamicAudioNormalizer**>(handle))
		{
			try
			{
				delete (*instance);
				(*instance) = NULL;
			}
			catch(...)
			{
				/*ignore exception*/
			}
		}
	}
	
	int  MDYNAMICAUDIONORMALIZER_FUNCTION(process)(MDynamicAudioNormalizer_Handle *const handle, const double *const *const samplesIn, double *const *const samplesOut, const int64_t inputSize, int64_t *const outputSize)
	{
		if (MDynamicAudioNormalizer *const instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
		{
			try
			{
				return instance->process(samplesIn, samplesOut, inputSize, (*outputSize)) ? 1 : 0;
			}
			catch (...)
			{
				return 0;
			}
		}
		return 0;
	}

	int MDYNAMICAUDIONORMALIZER_FUNCTION(processInplace)(MDynamicAudioNormalizer_Handle *const handle, double *const *const samplesInOut, const int64_t inputSize, int64_t *const outputSize)
	{
		if(MDynamicAudioNormalizer *const instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
		{
			try
			{
				return instance->processInplace(samplesInOut, inputSize, (*outputSize)) ? 1 : 0;
			}
			catch(...)
			{
				return 0;
			}
		}
		return 0;
	}

	int  MDYNAMICAUDIONORMALIZER_FUNCTION(flushBuffer)(MDynamicAudioNormalizer_Handle *const handle, double *const *const samplesOut, const int64_t bufferSize, int64_t *const outputSize)
	{
		if(MDynamicAudioNormalizer *const instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
		{
			try
			{
				return instance->flushBuffer(samplesOut, bufferSize, (*outputSize)) ? 1 : 0;
			}
			catch(...)
			{
				return 0;
			}
		}
		return 0;
	}

	int MDYNAMICAUDIONORMALIZER_FUNCTION(reset)(MDynamicAudioNormalizer_Handle *const handle)
	{
		if(MDynamicAudioNormalizer *instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
		{
			try
			{
				return instance->reset() ? 1 : 0;
			}
			catch(...)
			{
				return 0;
			}
		}
		return 0;
	}

	int MDYNAMICAUDIONORMALIZER_FUNCTION(getConfiguration)(MDynamicAudioNormalizer_Handle *const handle, uint32_t *const channels, uint32_t *const sampleRate, uint32_t *const frameLen, uint32_t *const filterSize)
	{
		if(MDynamicAudioNormalizer *const instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
		{
			try
			{
				return instance->getConfiguration((*channels), (*sampleRate), (*frameLen), (*filterSize)) ? 1 : 0;
			}
			catch(...)
			{
				return 0;
			}
		}
		return 0;
	}

	int MDYNAMICAUDIONORMALIZER_FUNCTION(getInternalDelay)(MDynamicAudioNormalizer_Handle *const handle, int64_t *const delayInSamples)
	{
		if(MDynamicAudioNormalizer *const instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
		{
			try
			{
				return instance->getInternalDelay(*delayInSamples) ? 1 : 0;
			}
			catch(...)
			{
				return 0;
			}
		}
		return 0;
	}

	MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction) *MDYNAMICAUDIONORMALIZER_FUNCTION(setLogFunction)(MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction) *const logFunction)
	{
		return MDynamicAudioNormalizer::setLogFunction(logFunction);
	}

	void MDYNAMICAUDIONORMALIZER_FUNCTION(getVersionInfo)(uint32_t *const major, uint32_t *const minor, uint32_t *const patch)
	{
		MDynamicAudioNormalizer::getVersionInfo((*major), (*minor), (*patch));
	}

	void MDYNAMICAUDIONORMALIZER_FUNCTION(getBuildInfo)(const char **const date, const char **const time, const char **const compiler, const char **const arch, int *const debug)
	{
		bool isDebug;
		MDynamicAudioNormalizer::getBuildInfo(date, time, compiler, arch, isDebug);
		*debug = isDebug ? 1 : 0;
	}
}
