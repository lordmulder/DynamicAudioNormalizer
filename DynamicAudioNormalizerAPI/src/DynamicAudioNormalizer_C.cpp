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

#include "DynamicAudioNormalizer.h"

extern "C"
{
	MDynamicAudioNormalizer_Handle* MDYNAMICAUDIONORMALIZER_FUNCTION(createInstance) (const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose, FILE *const logFile)
	{
		try
		{
			MDynamicAudioNormalizer *instance = new MDynamicAudioNormalizer(channels, sampleRate, frameLenMsec, channelsCoupled, enableDCCorrection, peakValue, maxAmplification, filterSize,verbose, logFile);
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

	MDYNAMICAUDIONORMALIZER_DLL void MDYNAMICAUDIONORMALIZER_FUNCTION(destroyInstance)(MDynamicAudioNormalizer_Handle **handle)
	{
		if(MDynamicAudioNormalizer *instance = reinterpret_cast<MDynamicAudioNormalizer*>(*handle))
		{
			try
			{
				delete instance;
				instance = NULL;
			}
			catch(...)
			{
				/*ignore exception*/
			}
			*handle = NULL;
		}
	}

	int MDYNAMICAUDIONORMALIZER_FUNCTION(processInplace)(MDynamicAudioNormalizer_Handle *handle, double **samplesInOut, int64_t inputSize, int64_t *outputSize)
	{
		if(MDynamicAudioNormalizer *instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
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

	int MDYNAMICAUDIONORMALIZER_FUNCTION(setPass)(MDynamicAudioNormalizer_Handle *handle, const int pass)
	{
		if(MDynamicAudioNormalizer *instance = reinterpret_cast<MDynamicAudioNormalizer*>(handle))
		{
			try
			{
				return instance->setPass(pass) ? 1 : 0;
			}
			catch(...)
			{
				return 0;
			}
		}
		return 0;
	}

	void MDYNAMICAUDIONORMALIZER_FUNCTION(getVersionInfo)(uint32_t *major, uint32_t *minor,uint32_t *patch)
	{
		MDynamicAudioNormalizer::getVersionInfo((*major), (*minor), (*patch));
	}

	void MDYNAMICAUDIONORMALIZER_FUNCTION(getBuildInfo)(const char **date, const char **time, const char **compiler, const char **arch, bool *debug)
	{
		MDynamicAudioNormalizer::getBuildInfo(date, time, compiler, arch, (*debug));
	}
}
