//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - VST 2.x Wrapper
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// http://opensource.org/licenses/MIT
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// ACKNOWLEDGEMENT:
// (1) VST PlugIn Interface Technology by Steinberg Media Technologies GmbH.
// (2) VST is a trademark of Steinberg Media Technologies GmbH.
//////////////////////////////////////////////////////////////////////////////////

#include "DynamicAudioNormalizerVST.h"

//Dynamic Audio Normalizer API
#include "DynamicAudioNormalizer.h"

//Internal
#include <Common.h>

//Standard Library
#include <cmath>
#include <algorithm>

//Win32 API
#ifdef _WIN32
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

//PThread
#if defined(_WIN32) && defined(_MT)
#define PTW32_STATIC_LIB 1
#endif
#include <pthread.h>

//Constants
static const VstInt32 CHANNEL_COUNT   = 2;
static const VstInt32 PARAMETER_COUNT = 8;
static const VstInt32 PROGRAM_COUNT   = 8;

//Default
static const char *DEFAULT_NAME = "#$!__DEFAULT__!$#";

//Critical Section
static char g_loggingBuffer[1024];
static pthread_mutex_t g_loggingMutex = PTHREAD_MUTEX_INITIALIZER;

//Enumerations
enum
{
	PARAM_REALVAL = 0,
	PARAM_INTEGER = 1,
	PARAM_BOOLEAN = 2
}
param_type_t;

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

static void outputMessage(const char *const format, ...)
{
	MY_CRITSEC_ENTER(g_loggingMutex);

	va_list argList;
	va_start(argList, format);
	vsnprintf_s(g_loggingBuffer, 1024, _TRUNCATE, format, argList);
	va_end(argList);
	OutputDebugStringA(g_loggingBuffer);

	MY_CRITSEC_LEAVE(g_loggingMutex);
}

static void logFunction(const int logLevel, const char *const message)
{
	switch (logLevel) 
	{
	case MDynamicAudioNormalizer::LOG_LEVEL_NFO:
		outputMessage("[DynAudNorm] NFO: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_WRN:
		outputMessage("[DynAudNorm] WAR: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_ERR:
		outputMessage("[DynAudNorm] ERR: %s\n", message);
		break;
	default:
		outputMessage("[DynAudNorm] DBG: %s\n", message);
		break;
	}
}

static void showErrorMsg(const char *const text)
{
	MessageBoxA(NULL, text, "Dynamic Audio Normalizer", MB_ICONSTOP | MB_TOPMOST);
}

static double applyStepSize(const double &val, const double &step)
{
	return (round(val / step) * step);
}

static double param2value(const double &min, const double &val, const double &max, const double &stepSize)
{
	const double offset = std::min(1.0, std::max(0.0, val)) * (max - min);
	return min + ((stepSize > DBL_EPSILON) ? applyStepSize(offset, stepSize) : offset);
}

static float value2param(const double &min, const double &val, const double &max)
{
	const double targt = std::min(max, std::max(min, double(val)));
	return static_cast<float>((targt - min) / (max - min));
}

static void value2string(char *text, const double &val, const double &min, const int &type, const bool &minDisable)
{
	if(minDisable && (fabs(val - min) < DBL_EPSILON))
	{
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "(OFF)");
		return;
	}

	switch(type)
	{
	case PARAM_REALVAL:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%.2f", val);
		break;
	case PARAM_INTEGER:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%d", static_cast<int>(val));
		break;
	case PARAM_BOOLEAN:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "%s", (val >= 0.5) ? "ON" : "OFF");
		break;
	default:
		_snprintf_s(text, kVstMaxParamStrLen, _TRUNCATE, "N/A");
	}
}

static uint32_t value2integer(const double &val) { return static_cast<uint32_t>(round(val)); }
static bool     value2boolean(const double &val) { return (val >= 0.5); }

///////////////////////////////////////////////////////////////////////////////
// Program Class
///////////////////////////////////////////////////////////////////////////////

struct DynamicAudioNormalizerVST_Program
{
	char name[kVstMaxProgNameLen + 1];
	float param[PARAMETER_COUNT];
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
	size_t tempSize;
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
	outputMessage("DynamicAudioNormalizerVST::DynamicAudioNormalizerVST()");

	setNumInputs(CHANNEL_COUNT);	// stereo in
	setNumOutputs(CHANNEL_COUNT);	// stereo out
	setUniqueID(uniqueId);			// identify
	canProcessReplacing();			// supports replacing output
	canDoubleReplacing();			// supports double precision processing
	noTail(false);					// does have Tail!

	p->programs = new DynamicAudioNormalizerVST_Program[PROGRAM_COUNT];
	for(VstInt32 i = 0; i < PROGRAM_COUNT; i++)
	{
		vst_strncpy(p->programs[i].name, DEFAULT_NAME, kVstMaxProgNameLen);
		for(VstInt32 j = 0; j < PARAMETER_COUNT; j++)
		{
			p->programs[i].param[j] = getParameterDefault(j);
		}
	}

	setProgram(0);
}

DynamicAudioNormalizerVST::~DynamicAudioNormalizerVST()
{
	outputMessage("DynamicAudioNormalizerVST::~DynamicAudioNormalizerVST()");

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
// Program Handling
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::setProgram(VstInt32 program)
{
	outputMessage("DynamicAudioNormalizerVST::setProgramName(%d)", program);
	AudioEffectX::setProgram(std::min(PROGRAM_COUNT-1, std::max(0, program)));
	forceUpdateParameters();
}

void DynamicAudioNormalizerVST::setProgramName (char* name)
{
	outputMessage("DynamicAudioNormalizerVST::setProgramName()");
	vst_strncpy(p->programs[curProgram].name, name, kVstMaxProgNameLen);
}

void DynamicAudioNormalizerVST::getProgramName (char* name)
{
	outputMessage("DynamicAudioNormalizerVST::getProgramName()");

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
	outputMessage("DynamicAudioNormalizerVST::getProgramNameIndexed(%d)", index);

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

///////////////////////////////////////////////////////////////////////////////
// Parameter Handling
///////////////////////////////////////////////////////////////////////////////

static const struct
{
	const char *paramName;
	const char *labelName;
	const double minValue;
	const double defValue;
	const double maxValue;
	const int contentType;
	const bool minDisable;
	const double stepSize;
}
PARAM_PROPERTIES[PARAMETER_COUNT] =
{
	{ "FrameLen", "MSec",   100.00, 500.00, 1000.00, PARAM_INTEGER, false, 1.00 },
	{ "FltrSize", "Frames",   3.00,  31.00,   63.00, PARAM_INTEGER, false, 2.00 },
	{ "PeakVal",  "%",       10.00,  95.00,  100.00, PARAM_REALVAL, false, 0.00 },
	{ "MaxAmpl",  "x",        1.00,  10.00,   50.00, PARAM_REALVAL, false, 0.00 },
	{ "TrgtRMS",  "%",        0.00,   0.00,  100.00, PARAM_REALVAL, true,  0.00 },
	{ "Compress", "sigma",    0.00,   0.00,   25.00, PARAM_REALVAL, true,  0.00 },
	{ "ChanCpld", "",         0.00,   1.00,    1.00, PARAM_BOOLEAN, false, 1.00 },
	{ "DCCorrct", "",         0.00,   0.00,    1.00, PARAM_BOOLEAN, false, 1.00 }
};

void DynamicAudioNormalizerVST::getParameterName (VstInt32 index, char* label)
{
	outputMessage("DynamicAudioNormalizerVST::getParameterName(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		vst_strncpy(label, PARAM_PROPERTIES[index].paramName, kVstMaxParamStrLen);
	}
	else
	{
		vst_strncpy(label, "N/A", kVstMaxParamStrLen);
	}
}

void DynamicAudioNormalizerVST::getParameterLabel (VstInt32 index, char* label)
{
	outputMessage("DynamicAudioNormalizerVST::getParameterLabel(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		vst_strncpy(label, PARAM_PROPERTIES[index].labelName, kVstMaxParamStrLen);
	}
	else
	{
		vst_strncpy(label, "N/A", kVstMaxParamStrLen);
	}
}

void DynamicAudioNormalizerVST::setParameter (VstInt32 index, float value)
{
	outputMessage("DynamicAudioNormalizerVST::setParameter(%d, %.2f)", index, value);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		p->programs[curProgram].param[index] = std::min(1.0f, std::max(0.0f, value));
	}
}

float DynamicAudioNormalizerVST::getParameter(VstInt32 index)
{
	outputMessage("DynamicAudioNormalizerVST::getParameter(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		return p->programs[curProgram].param[index];
	}

	return -1.0;
}

double DynamicAudioNormalizerVST::getParameterValue(VstInt32 index)
{
	outputMessage("DynamicAudioNormalizerVST::getParameterValue(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		
		return param2value(PARAM_PROPERTIES[index].minValue, p->programs[curProgram].param[index], PARAM_PROPERTIES[index].maxValue, PARAM_PROPERTIES[index].stepSize);
	}

	return -1.0;
}

float DynamicAudioNormalizerVST::getParameterDefault(VstInt32 index)
{
	outputMessage("DynamicAudioNormalizerVST::getParameterDefault(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		
		return value2param(PARAM_PROPERTIES[index].minValue, PARAM_PROPERTIES[index].defValue, PARAM_PROPERTIES[index].maxValue);
	}

	return -1.0;
}

void DynamicAudioNormalizerVST::getParameterDisplay(VstInt32 index, char* text)
{
	outputMessage("DynamicAudioNormalizerVST::getParameterDisplay(%d)", index);

	text[0] = '\0';
	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		value2string(text, getParameterValue(index), PARAM_PROPERTIES[index].minValue, PARAM_PROPERTIES[index].contentType, PARAM_PROPERTIES[index].minDisable);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Effect Info
///////////////////////////////////////////////////////////////////////////////

bool DynamicAudioNormalizerVST::getEffectName (char* name)
{
	outputMessage("DynamicAudioNormalizerVST::getEffectName()");

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	_snprintf_s(name, kVstMaxEffectNameLen, _TRUNCATE, "DynamicAudioNormalizer %u.%02u-%u", major, minor, patch);
	return true;
}

bool DynamicAudioNormalizerVST::getProductString (char* text)
{
	outputMessage("DynamicAudioNormalizerVST::getProductString()");

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	_snprintf_s(text, kVstMaxProductStrLen, _TRUNCATE, "DynamicAudioNormalizer_%u", (100U * major) + minor);
	return true;
}

bool DynamicAudioNormalizerVST::getVendorString (char* text)
{
	outputMessage("DynamicAudioNormalizerVST::getVendorString()");

	static const char *vendorName = "LoRd_MuldeR";
	_snprintf_s(text, kVstMaxVendorStrLen, _TRUNCATE, "%s", vendorName);
	return true;
}

VstInt32 DynamicAudioNormalizerVST::getVendorVersion ()
{ 
	outputMessage("DynamicAudioNormalizerVST::getVendorVersion()");

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	return (major * 1000U) + (minor * 10U) + patch;
}

VstPlugCategory DynamicAudioNormalizerVST::getPlugCategory(void)
{
	outputMessage("DynamicAudioNormalizerVST::getPlugCategory()");
	return kPlugCategEffect;
}

///////////////////////////////////////////////////////////////////////////////
// State Handling
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::open(void)
{
	outputMessage("DynamicAudioNormalizerVST::open()");
	forceUpdateParameters();
}

void DynamicAudioNormalizerVST::close(void)
{
	outputMessage("DynamicAudioNormalizerVST::close()");
}

void DynamicAudioNormalizerVST::resume(void)
{
	outputMessage("DynamicAudioNormalizerVST::resume()");

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
	outputMessage("DynamicAudioNormalizerVST::suspend()");

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
	outputMessage("DynamicAudioNormalizerVST::getGetTailSize()");

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

bool DynamicAudioNormalizerVST::createNewInstance(const uint32_t sampleRate)
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
			value2integer(getParameterValue(0)),
			value2integer(getParameterValue(1)),
			getParameterValue(2) / 100.0,
			getParameterValue(3),
			getParameterValue(4) / 100.0,
			getParameterValue(5),
			value2boolean(getParameterValue(6)),
			value2boolean(getParameterValue(7))
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

void DynamicAudioNormalizerVST::forceUpdateParameters(void)
{
	for(VstInt32 i = 0; i < PARAMETER_COUNT; i++)
	{
		beginEdit(i);
		setParameterAutomated(i, p->programs[curProgram].param[i]);
		endEdit(i);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Create Instance
///////////////////////////////////////////////////////////////////////////////

static bool g_initialized = false;
static pthread_mutex_t g_createEffMutex = PTHREAD_MUTEX_INITIALIZER;

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	MY_CRITSEC_ENTER(g_createEffMutex);
	AudioEffect *effectInstance = NULL;

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	outputMessage("Dynamic Audio Normalizer VST-Wrapper (v%u.%02u-%u)", major, minor, patch);
	
	if(!g_initialized)
	{
		MDynamicAudioNormalizer::setLogFunction(logFunction);
		g_initialized = true;
	}
	
	effectInstance = new DynamicAudioNormalizerVST(audioMaster);
	
	MY_CRITSEC_LEAVE(g_createEffMutex);
	return effectInstance;
}
