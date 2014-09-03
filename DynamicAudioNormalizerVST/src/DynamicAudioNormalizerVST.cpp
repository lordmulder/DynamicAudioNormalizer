//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - VST 2.x Wrapper
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

//////////////////////////////////////////////////////////////////////////////////
// ACKNOWLEDGEMENT:
// (1) VST PlugIn Interface Technology by Steinberg Media Technologies GmbH.
// (2) VST is a trademark of Steinberg Media Technologies GmbH.
//////////////////////////////////////////////////////////////////////////////////

#include "DynamicAudioNormalizerVST.h"

//Dynamic Audio Normalizer API
#include "DynamicAudioNormalizer.h"

//Standard Library
#include <cmath>
#include <algorithm>

//Win32 API
#ifdef _WIN32
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

//Constants
static const VstInt32 CHANNEL_COUNT = 2;
static const VstInt32 PARAMETER_COUNT = 8;
static const VstInt32 PROGRAM_COUNT = 8;

//Default
static const char *DEFAULT_NAME = "#$!__DEFAULT__!$#";

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

__declspec(thread) static char g_logBuffer[1024];

static void logFunction(const int logLevel, const char *const message)
{
	switch (logLevel) 
	{
	case MDynamicAudioNormalizer::LOG_LEVEL_NFO:
		_snprintf_s(g_logBuffer, 1024, _TRUNCATE, "[DynAudNorm] NFO: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_WRN:
		_snprintf_s(g_logBuffer, 1024, _TRUNCATE, "[DynAudNorm] WAR: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_ERR:
		_snprintf_s(g_logBuffer, 1024, _TRUNCATE, "[DynAudNorm] ERR: %s\n", message);
		break;
	default:
		_snprintf_s(g_logBuffer, 1024, _TRUNCATE, "[DynAudNorm] DBG: %s\n", message);
		break;
	}

	OutputDebugStringA(g_logBuffer);
}

static void showErrorMsg(const char *const text)
{
	MessageBoxA(NULL, text, "Dynamic Audio Normalizer", MB_ICONSTOP | MB_TOPMOST);
}

///////////////////////////////////////////////////////////////////////////////
// Program Class
///////////////////////////////////////////////////////////////////////////////

class DynamicAudioNormalizerVST_Program
{
	friend class DynamicAudioNormalizerVST;

public:
	DynamicAudioNormalizerVST_Program()
	{
		frameLenMsec       = 0.5;
		filterSize         = 0.5;
		peakValue          = 0.9444;
		maxAmplification   = 0.4595;
		targetRms          = 0.0;
		compressThresh     = 0.0;
		channelsCoupled    = 1.0;
		enableDCCorrection = 0.0;

		vst_strncpy(name, DEFAULT_NAME, kVstMaxProgNameLen);
	}

	~DynamicAudioNormalizerVST_Program()
	{
	}

	uint32_t getFrameLen(void) const
	{
		return static_cast<uint32_t>(round(50.0 + (900.0 * frameLenMsec)));
	}
	
	uint32_t getFilterSize(void) const
	{
		return 1U + (static_cast<uint32_t>(round(1.0 + (28.0 * filterSize))) * 2U);
	}

	double getPeakValue(void) const
	{
		return 0.1 + (0.9 * peakValue);
	}

	double getMaxAmplification(void) const
	{
		return 1.5 + (18.5 * maxAmplification);
	}

	double getTargetRms(void) const
	{
		return (targetRms > DBL_EPSILON) ? (0.1 + (0.9 * targetRms)) : 0.0;
	}

	double getCompressThresh(void) const
	{
		return (compressThresh > DBL_EPSILON) ? (0.1 + (0.9 * compressThresh)) : 0.0;
	}

	bool getChannelsCoupled(void) const
	{
		return channelsCoupled >= 0.5;
	}

	bool getEnableDCCorrection(void) const
	{
		return enableDCCorrection >= 0.5;
	}

private:
	char name[kVstMaxProgNameLen + 1];

	float frameLenMsec;
	float filterSize;
	float peakValue;
	float maxAmplification;
	float targetRms;
	float compressThresh;
	float channelsCoupled;
	float enableDCCorrection;
};

///////////////////////////////////////////////////////////////////////////////
// Private Data
///////////////////////////////////////////////////////////////////////////////

class DynamicAudioNormalizerVST_PrivateData
{
	friend class DynamicAudioNormalizerVST;

public:
	DynamicAudioNormalizerVST_PrivateData(void)
	{
		this->instance = NULL;
		this->programs = NULL;
		this->temp[0] = this->temp[1] = NULL;
		this->tempSize = 0;
	}

	~DynamicAudioNormalizerVST_PrivateData(void)
	{
	}

protected:
	MDynamicAudioNormalizer *instance;
	DynamicAudioNormalizerVST_Program *programs;
	double *temp[2];
	uint32_t tempSize;
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

DynamicAudioNormalizerVST::DynamicAudioNormalizerVST(audioMasterCallback audioMaster)
:
	AudioEffectX(audioMaster, PROGRAM_COUNT, PARAMETER_COUNT),
	p(new DynamicAudioNormalizerVST_PrivateData())
{
	static const VstInt32 uniqueId = CCONST('!','c','2','N');

	setNumInputs(CHANNEL_COUNT);	// stereo in
	setNumOutputs(CHANNEL_COUNT);	// stereo out
	setUniqueID(uniqueId);			// identify
	canProcessReplacing();			// supports replacing output
	canDoubleReplacing();			// supports double precision processing
	noTail(false);					// does have Tail!

	setProgram(0);
	p->programs = new DynamicAudioNormalizerVST_Program[PROGRAM_COUNT];
}

DynamicAudioNormalizerVST::~DynamicAudioNormalizerVST()
{
	if(p->instance)
	{
		delete p->instance;
		p->instance = NULL;
	}

	if(p->tempSize > 0)
	{
		delete [] p->temp[0]; p->temp[0] = NULL;
		delete [] p->temp[1]; p->temp[1] = NULL;
		p->tempSize = 0;
	}

	if (p->programs)
	{
		delete[] p->programs;
		p->programs = NULL;
	}

	delete p;
}

///////////////////////////////////////////////////////////////////////////////
// Program and Parameter Handling
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::setProgram(VstInt32 program)
{
	AudioEffectX::setProgram(std::min(PROGRAM_COUNT-1, std::max(0, program)));
}

void DynamicAudioNormalizerVST::setProgramName (char* name)
{
	vst_strncpy(p->programs[curProgram].name, name, kVstMaxProgNameLen);
}

void DynamicAudioNormalizerVST::getProgramName (char* name)
{
	if(strcmp(p->programs[curProgram].name, DEFAULT_NAME) == 0)
	{
		_snprintf_s(name, kVstMaxProgNameLen, _TRUNCATE, "Preset #%u", curProgram + 1);
	}
	else
	{
		vst_strncpy(name, p->programs[curProgram].name, kVstMaxProgNameLen);
	}
}

bool DynamicAudioNormalizerVST::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
	if ((index >= 0) && (index < PROGRAM_COUNT))
	{
		if (strcmp(p->programs[index].name, DEFAULT_NAME) == 0)
		{
			_snprintf_s(text, kVstMaxProgNameLen, _TRUNCATE, "Preset #%u", index + 1);
		}
		else
		{
			vst_strncpy(text, p->programs[index].name, kVstMaxProgNameLen);
		}
		return true;
	}
	else
	{
		vst_strncpy(text, "N/A", kVstMaxProgNameLen);
		return false;
	}
}

void DynamicAudioNormalizerVST::setParameter (VstInt32 index, float value)
{
	switch (index)
	{
		case 0: p->programs[curProgram].frameLenMsec = value;       break;
		case 1: p->programs[curProgram].filterSize = value;         break;
		case 2: p->programs[curProgram].peakValue = value;          break;
		case 3: p->programs[curProgram].maxAmplification = value;   break;
		case 4: p->programs[curProgram].targetRms = value;          break;
		case 5: p->programs[curProgram].compressThresh = value;     break;
		case 6: p->programs[curProgram].channelsCoupled = value;    break;
		case 7: p->programs[curProgram].enableDCCorrection = value; break;
	}
}

float DynamicAudioNormalizerVST::getParameter (VstInt32 index)
{
	switch (index)
	{
		case 0: return p->programs[curProgram].frameLenMsec;
		case 1: return p->programs[curProgram].filterSize;
		case 2: return p->programs[curProgram].peakValue;
		case 3: return p->programs[curProgram].maxAmplification;
		case 4: return p->programs[curProgram].targetRms;
		case 5: return p->programs[curProgram].compressThresh;
		case 6: return p->programs[curProgram].channelsCoupled;
		case 7: return p->programs[curProgram].enableDCCorrection;
	}

	return 0.0;
}

void DynamicAudioNormalizerVST::getParameterName (VstInt32 index, char* label)
{
	static const char *NAMES[PARAMETER_COUNT] =
	{
		"FrameLen",
		"FltrSize",
		"PeakVal",
		"MaxAmpl",
		"TrgtRMS",
		"Compress",
		"ChanCpld",
		"DCCorrct"
	};

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		vst_strncpy(label, NAMES[index], kVstMaxParamStrLen);
	}
	else
	{
		vst_strncpy(label, "N/A", kVstMaxParamStrLen);
	}
}

void DynamicAudioNormalizerVST::getParameterDisplay(VstInt32 index, char* text)
{
	text[0] = '\0';

	switch (index)
	{
	case 0:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%u", p->programs[curProgram].getFrameLen());
		break;
	case 1:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%u", p->programs[curProgram].getFilterSize());
		break;
	case 2:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%.1f", 100.0 * p->programs[curProgram].getPeakValue());
		break;
	case 3:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%.2f", p->programs[curProgram].getMaxAmplification());
		break;
	case 4:
		if(const double targetRms = p->programs[curProgram].targetRms)
		{
			_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%.1f", 100.0 * targetRms);
		}
		else
		{
			_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "(OFF)");
		}
		break;
	case 5:
		if(const double compressThresh = p->programs[curProgram].getCompressThresh())
		{
			_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%.1f", 100.0 * compressThresh);
		}
		else
		{
			_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "(OFF)");
		}
		break;
	case 6:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%s", p->programs[curProgram].getChannelsCoupled() ? "ON" : "OFF");
		break;
	case 7:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%s", p->programs[curProgram].getEnableDCCorrection() ? "ON" : "OFF");
		break;
	}
}

void DynamicAudioNormalizerVST::getParameterLabel (VstInt32 index, char* label)
{
	static const char *LABEL[PARAMETER_COUNT] =
	{
		"MSec",
		"Frames",
		"%",
		"x",
		"%",
		"%",
		"",
		""
	};

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		vst_strncpy(label, LABEL[index], kVstMaxParamStrLen);
	}
	else
	{
		vst_strncpy(label, "N/A", kVstMaxParamStrLen);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Effect Info
///////////////////////////////////////////////////////////////////////////////

bool DynamicAudioNormalizerVST::getEffectName (char* name)
{
	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	_snprintf_s(name, kVstMaxEffectNameLen, _TRUNCATE, "DynamicAudioNormalizer %u.%02u-%u", major, minor, patch);
	return true;
}

bool DynamicAudioNormalizerVST::getProductString (char* text)
{
	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	_snprintf_s(text, kVstMaxProductStrLen, _TRUNCATE, "DynamicAudioNormalizer_%u", (100U * major) + minor);
	return true;
}

bool DynamicAudioNormalizerVST::getVendorString (char* text)
{
	vst_strncpy(text, "LoRd_MuldeR <mulder2@gmxde>", kVstMaxVendorStrLen);
	return true;
}

VstInt32 DynamicAudioNormalizerVST::getVendorVersion ()
{ 
	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);

	return (major * 1000U) + (minor * 10U) + patch;
}

///////////////////////////////////////////////////////////////////////////////
// State Handling
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::open(void)
{
	logFunction(-1, "DynamicAudioNormalizerVST::open()");
}

void DynamicAudioNormalizerVST::close(void)
{
	logFunction(-1, "DynamicAudioNormalizerVST::close()");
}

void DynamicAudioNormalizerVST::resume(void)
{
	logFunction(-1, "DynamicAudioNormalizerVST::resume()");

	if (createNewInstance(static_cast<uint32_t>(round(getSampleRate()))))
	{
		int64_t delayInSamples;
		if(p->instance->getInternalDelay(delayInSamples))
		{
			setInitialDelay(static_cast<VstInt32>(std::min(delayInSamples, int64_t(INT32_MAX))));
		}
	}
}

void DynamicAudioNormalizerVST::suspend(void)
{
	logFunction(-1, "DynamicAudioNormalizerVST::suspend()");

	if(p->instance)
	{
		delete p->instance;
		p->instance = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Audio Processing
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	if(sampleFrames < 1)
	{
		return; /*number of samples is zero or even negative!*/
	}

	if(!p->instance)
	{
		showErrorMsg("Dynamic Audio Normalizer instance not created yet!");
		return;
	}

	updateBufferSize(sampleFrames);
	readInputSamplesFlt(inputs, sampleFrames);

	int64_t outputSamples;
	if(!p->instance->processInplace(p->temp, sampleFrames, outputSamples))
	{
		showErrorMsg("Dynamic Audio Normalizer processing failed!");
		return;
	}

	writeOutputSamplesFlt(outputs, sampleFrames, outputSamples);
}

void DynamicAudioNormalizerVST::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
	if(sampleFrames < 1)
	{
		return; /*number of samples is zero or even negative!*/
	}

	if(!p->instance)
	{
		showErrorMsg("Dynamic Audio Normalizer instance not created yet!");
		return;
	}

	updateBufferSize(sampleFrames);
	readInputSamplesDbl(inputs, sampleFrames);

	int64_t outputSamples;
	if(!p->instance->processInplace(p->temp, sampleFrames, outputSamples))
	{
		showErrorMsg("Dynamic Audio Normalizer processing failed!");
		return;
	}

	writeOutputSamplesDbl(outputs, sampleFrames, outputSamples);
}

VstInt32 DynamicAudioNormalizerVST::getGetTailSize(void)
{
	if(p->instance)
	{
		int64_t delayInSamples;
		if(p->instance->getInternalDelay(delayInSamples))
		{
			return static_cast<VstInt32>(delayInSamples);
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

bool  DynamicAudioNormalizerVST::createNewInstance(const uint32_t sampleRate)
{
	if(p->instance)
	{
		showErrorMsg("Dynamic Audio Normalizer instance already created!");
		return true;
	}
	
	const DynamicAudioNormalizerVST_Program *const currentProg = &p->programs[curProgram];

	try
	{
		p->instance = new MDynamicAudioNormalizer
		(
			CHANNEL_COUNT,
			sampleRate,
			currentProg->getFrameLen(),
			currentProg->getFilterSize(),
			currentProg->getPeakValue(),
			currentProg->getMaxAmplification(),
			currentProg->getTargetRms(),
			currentProg->getCompressThresh(),
			currentProg->getChannelsCoupled(),
			currentProg->getEnableDCCorrection()
		);
	}
	catch(...)
	{
		showErrorMsg("Failed to create Dynamic Audio Normalizer instance!");
		return false;
	}

	if(!p->instance->initialize())
	{
		showErrorMsg("Dynamic Audio Normalizer initialization has failed!");
		return false;
	}

	return true;
}

void DynamicAudioNormalizerVST::updateBufferSize(const size_t requiredSize)
{
	if(p->tempSize < requiredSize)
	{
		if(p->tempSize > 0)
		{
			delete [] p->temp[0];
			delete [] p->temp[0];
		}
		try
		{
			p->temp[0] = new double[requiredSize];
			p->temp[1] = new double[requiredSize];
		}
		catch(...)
		{
			showErrorMsg("Memory allocation has failed: Out of memory!");
			_exit(0xC0000017);
		}
		p->tempSize = requiredSize;
	}
}

void DynamicAudioNormalizerVST::readInputSamplesFlt(const float *const *const inputs, const int64_t sampleCount)
{
	for(int64_t i = 0; i < sampleCount; i++)
	{
		p->temp[0][i] = inputs[0][i];
		p->temp[1][i] = inputs[1][i];
	}
}

void DynamicAudioNormalizerVST::readInputSamplesDbl(const double *const *const inputs, const int64_t sampleCount)
{
	for(int64_t i = 0; i < sampleCount; i++)
	{
		p->temp[0][i] = inputs[0][i];
		p->temp[1][i] = inputs[1][i];
	}
}

void DynamicAudioNormalizerVST::writeOutputSamplesFlt(float *const *const outputs, const int64_t sampleCount, const int64_t outputSamples)
{
	if(outputSamples == 0)
	{
		for(int64_t i = 0; i < sampleCount; i++)
		{
			outputs[0][i] = 0.0f;
			outputs[1][i] = 0.0f;
		}
	}
	else if(outputSamples < sampleCount)
	{
		const int64_t offset = sampleCount - outputSamples;
		int64_t pos0 = 0, pos1 = 0;
		for(int64_t i = 0; i < offset; i++)
		{
			outputs[0][pos0++] = 0.0f;
			outputs[1][pos1++] = 0.0f;
		}
		for(int64_t i = 0; i < outputSamples; i++)
		{
			outputs[0][pos0++] = static_cast<float>(p->temp[0][i]);
			outputs[1][pos1++] = static_cast<float>(p->temp[1][i]);
		}
	}
	else if(outputSamples == sampleCount)
	{
		for(int64_t i = 0; i < outputSamples; i++)
		{
			outputs[0][i] = static_cast<float>(p->temp[0][i]);
			outputs[1][i] = static_cast<float>(p->temp[1][i]);
		}
	}
	else
	{
		showErrorMsg("Number of output samples exceeds output buffer size!");
	}
}

void DynamicAudioNormalizerVST::writeOutputSamplesDbl(double *const *const outputs, const int64_t sampleCount, const int64_t outputSamples)
{
	if(outputSamples == 0)
	{
		for(int64_t i = 0; i < sampleCount; i++)
		{
			outputs[0][i] = 0.0f;
			outputs[1][i] = 0.0f;
		}
	}
	else if(outputSamples < sampleCount)
	{
		const int64_t offset = sampleCount - outputSamples;
		int64_t pos0 = 0, pos1 = 0;
		for(int64_t i = 0; i < offset; i++)
		{
			outputs[0][pos0++] = 0.0f;
			outputs[1][pos1++] = 0.0f;
		}
		for(int64_t i = 0; i < outputSamples; i++)
		{
			outputs[0][pos0++] = p->temp[0][i];
			outputs[1][pos1++] = p->temp[1][i];
		}
	}
	else if(outputSamples == sampleCount)
	{
		for(int64_t i = 0; i < outputSamples; i++)
		{
			outputs[0][i] = p->temp[0][i];
			outputs[1][i] = p->temp[1][i];
		}
	}
	else
	{
		showErrorMsg("Number of output samples exceeds output buffer size!");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Create Instance
///////////////////////////////////////////////////////////////////////////////

static bool g_initialized = false;

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	if(!g_initialized)
	{
		MDynamicAudioNormalizer::setLogFunction(logFunction);
		g_initialized = true;
	}

	return new DynamicAudioNormalizerVST(audioMaster);
}
