//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - VST 2.x Wrapper
// Copyright (c) 2014-2018 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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
#include <DynamicAudioNormalizer.h>

//Internal
#include <Common.h>
#include <Threads.h>
#include "Spooky.h"

//Standard Library
#include <cmath>
#include <algorithm>
#include <inttypes.h>

//Win32 API
#ifdef _WIN32
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#define PSAPI_VERSION 1
#include <Windows.h>
#include <Psapi.h>
#endif

//Enable extra logging output?
#undef DEBUG_LOGGING

//Constants
static const VstInt32 CHANNEL_COUNT   = 2;
static const VstInt32 PARAMETER_COUNT = 8;
static const VstInt32 PROGRAM_COUNT   = 8;

//Default
static const char *DEFAULT_NAME = "#$!__DEFAULT__!$#";
static const size_t INITIAL_BUFFER_SIZE = 8192;

//Reg key
static const wchar_t *REGISTRY_PATH = L"Software\\MuldeR\\DynAudNorm\\VST";

//Critical Section
static char g_loggingBuffer[1024];
static MY_CRITSEC_INIT(g_loggingMutex);

//Enumerations
enum
{
	PARAM_REALVAL = 0,
	PARAM_INTEGER = 1,
	PARAM_BOOLEAN = 2
}
param_type_t;

//Seed values
#ifdef _M_X64
static const uint64_t SEED_VALU1 = 0xC11D908A35A625A5, SEED_VALU2 = 0x5E57D080BCD2EAED;
#else
static const uint64_t SEED_VALU1 = 0x24575239D116386B, SEED_VALU2 = 0xE03BDD5B975F47D0;
#endif

//Debugging outputs
#ifdef DEBUG_LOGGING
#define DEBUG_LOG(...) outputMessage(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif //DEBUG_LOGGING

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
	case MDynamicAudioNormalizer::LOG_LEVEL_DBG:
		outputMessage("[DynAudNorm_VST] NFO: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_WRN:
		outputMessage("[DynAudNorm_VST] WRN: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_ERR:
		outputMessage("[DynAudNorm_VST] ERR: %s\n", message);
		break;
	default:
		outputMessage("[DynAudNorm_VST] DBG: %s\n", message);
		break;
	}
}

static void showErrorMsg(const char *const text)
{
	MessageBoxA(NULL, text, "Dynamic Audio Normalizer", MB_ICONSTOP | MB_TOPMOST | MB_TASKMODAL);
}

static DWORD regValueGet(const wchar_t *const name)
{
	DWORD result = 0; HKEY hKey = NULL;
	if(RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, 0, KEY_READ, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		DWORD type = 0, value = 0, size = sizeof(DWORD);
		if(RegQueryValueExW(hKey, name, 0, &type, ((LPBYTE) &value), &size) == ERROR_SUCCESS)
		{
			if((type == REG_DWORD) && (size == sizeof(DWORD)))
			{
				result = value;
			}
		}
		RegCloseKey(hKey);
	}
	return result;
}

static bool regValueSet(const wchar_t *const name, const DWORD &value)
{
	bool result = false; HKEY hKey = NULL;
	if(RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		if(RegSetValueExW(hKey, name, 0, REG_DWORD, ((LPBYTE) &value), sizeof(DWORD)) == ERROR_SUCCESS)
		{
			result = true;
		}
		RegCloseKey(hKey);
	}
	return result;
}

static bool getAppUuid(wchar_t *const uuidOut, const size_t &size)
{
	if(size < 1)
	{
		return false;
	}

	wchar_t executableFilePath[512];
	if(GetProcessImageFileNameW(GetCurrentProcess(), executableFilePath, 512) < 1)
	{
		wcsncpy_s(executableFilePath, 512, L"Lorem ipsum dolor sit amet, consetetur sadipscing elitr!", _TRUNCATE);
	}

	uint64_t hash1 = SEED_VALU1, hash2 = SEED_VALU2;
	hash128(executableFilePath, &hash1, &hash2);

	_snwprintf_s(uuidOut, size, _TRUNCATE, L"{%016llX-%016llX}", hash1, hash2);
	return true;
}

static VstInt32 getPlugUuid(void)
{
	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);

	wchar_t identifier[128];
	_snwprintf_s(identifier, 128, _TRUNCATE, L"muldersoft.com::DynamicAudioNormalizerVST::%u.%02u.%u", major, minor, patch);

	uint64_t hash1 = SEED_VALU1, hash2 = SEED_VALU2;
	hash128(identifier, &hash1, &hash2);

	const VstInt32 hash32_pt1 = static_cast<VstInt32>(((hash1 ^ hash2) & 0xFFFFFFFF00000000ui64) >> 32);
	const VstInt32 hash32_pt2 = static_cast<VstInt32>((hash1 ^ hash2) & 0x00000000FFFFFFFFui64);
	
	return hash32_pt1 ^ hash32_pt2;
}

template <typename T> static T *allocBuffer(size_t size)
{
	T *buffer = NULL;
	try
	{
		buffer = new T[size];
	}
	catch(...)
	{
		outputMessage("[DynAudNorm_VST] ERR: Allocation of size %u failed!", static_cast<unsigned int>(sizeof(T) * size));
		showErrorMsg("Memory allocation has failed: Out of memory!");
		_exit(0xC0000017);
	}
	return buffer;
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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::DynamicAudioNormalizerVST()");
	const VstInt32 uniqueId = getPlugUuid();

	setNumInputs(CHANNEL_COUNT);	// stereo in
	setNumOutputs(CHANNEL_COUNT);	// stereo out
	setUniqueID(uniqueId);			// identify
	canProcessReplacing();			// supports replacing output
	canDoubleReplacing();			// supports double precision processing
	noTail(false);					// does have Tail!

	p->programs = allocBuffer<DynamicAudioNormalizerVST_Program>(PROGRAM_COUNT);
	for(VstInt32 i = 0; i < PROGRAM_COUNT; i++)
	{
		vst_strncpy(p->programs[i].name, DEFAULT_NAME, kVstMaxProgNameLen);
		for(VstInt32 j = 0; j < PARAMETER_COUNT; j++)
		{
			p->programs[i].param[j] = getParameterDefault(j);
		}
	}

	for(size_t i = 0; i < 2; i++)
	{
		p->temp[i] = allocBuffer<double>(INITIAL_BUFFER_SIZE);
	}
	
	p->tempSize = INITIAL_BUFFER_SIZE;
	setProgram(0);
}

DynamicAudioNormalizerVST::~DynamicAudioNormalizerVST()
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::~DynamicAudioNormalizerVST()");

	if(p->instance)
	{
		delete p->instance;
		p->instance = NULL;
	}

	if(p->tempSize > 0)
	{
		for(size_t i = 0; i < 2; i++)
		{
			delete [] (p->temp[i]);
			p->temp[i] = NULL;
		}
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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::setProgramName(%d)", program);
	AudioEffectX::setProgram(std::min(PROGRAM_COUNT-1, std::max(0, program)));
	forceUpdateParameters();
}

void DynamicAudioNormalizerVST::setProgramName (char* name)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::setProgramName()");
	vst_strncpy(p->programs[curProgram].name, name, kVstMaxProgNameLen);
}

void DynamicAudioNormalizerVST::getProgramName (char* name)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getProgramName()");

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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getProgramNameIndexed(%d)", index);

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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getParameterName(%d)", index);

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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getParameterLabel(%d)", index);

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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::setParameter(%d, %.2f)", index, value);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		p->programs[curProgram].param[index] = std::min(1.0f, std::max(0.0f, value));
	}
}

float DynamicAudioNormalizerVST::getParameter(VstInt32 index)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getParameter(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		return p->programs[curProgram].param[index];
	}

	return -1.0;
}

double DynamicAudioNormalizerVST::getParameterValue(VstInt32 index)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getParameterValue(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		
		return param2value(PARAM_PROPERTIES[index].minValue, p->programs[curProgram].param[index], PARAM_PROPERTIES[index].maxValue, PARAM_PROPERTIES[index].stepSize);
	}

	return -1.0;
}

float DynamicAudioNormalizerVST::getParameterDefault(VstInt32 index)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getParameterDefault(%d)", index);

	if ((index >= 0) && (index < PARAMETER_COUNT))
	{
		
		return value2param(PARAM_PROPERTIES[index].minValue, PARAM_PROPERTIES[index].defValue, PARAM_PROPERTIES[index].maxValue);
	}

	return -1.0;
}

void DynamicAudioNormalizerVST::getParameterDisplay(VstInt32 index, char* text)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getParameterDisplay(%d)", index);

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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getEffectName()");

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	_snprintf_s(name, kVstMaxEffectNameLen, _TRUNCATE, "DynamicAudioNormalizer %u.%02u-%u", major, minor, patch);
	return true;
}

bool DynamicAudioNormalizerVST::getProductString (char* text)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getProductString()");

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	_snprintf_s(text, kVstMaxProductStrLen, _TRUNCATE, "DynamicAudioNormalizer_%u", (100U * major) + minor);
	return true;
}

bool DynamicAudioNormalizerVST::getVendorString (char* text)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getVendorString()");

	static const char *vendorName = "LoRd_MuldeR";
	_snprintf_s(text, kVstMaxVendorStrLen, _TRUNCATE, "%s", vendorName);
	return true;
}

VstInt32 DynamicAudioNormalizerVST::getVendorVersion ()
{ 
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getVendorVersion()");

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	return (major * 1000U) + (minor * 10U) + patch;
}

VstPlugCategory DynamicAudioNormalizerVST::getPlugCategory(void)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getPlugCategory()");
	return kPlugCategEffect;
}

///////////////////////////////////////////////////////////////////////////////
// State Handling
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::open(void)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::open()");
	forceUpdateParameters();
}

void DynamicAudioNormalizerVST::close(void)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::close()");

	if(p->instance)
	{
		outputMessage("[DynAudNorm_VST] ISANITY:  Host called close() after resume() *without* intermittent susped() call -> workaround actived!");
		suspend();
	}
}

void DynamicAudioNormalizerVST::resume(void)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::resume()");

	if(p->instance)
	{
		outputMessage("[DynAudNorm_VST] ISANITY: Host called resume() repeatedly *without* intermittent susped() call -> workaround actived!");
		suspend();
	}

	if(createNewInstance(static_cast<uint32_t>(round(getSampleRate()))))
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
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::suspend()");

	if(p->instance)
	{
		delete p->instance;
		p->instance = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Audio Processing
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	DEBUG_LOG("[DynAudNorm_VST] ENTER >>> processReplacing()");

	if(sampleFrames < 1)
	{
		return; /*number of samples is zero or even negative!*/
	}

	if(!p->instance)
	{
		outputMessage("[DynAudNorm_VST] ISANITY: Host called processReplacing() *before* resume() has been called -> workaround actived!");
		resume();
	}

	updateBufferSize(sampleFrames);
	readInputSamplesFlt(inputs, sampleFrames);

	int64_t outputSamples;
	if(!p->instance->processInplace(p->temp, sampleFrames, outputSamples))
	{
		showErrorMsg("Dynamic Audio Normalizer processing failed!");
		DEBUG_LOG("[DynAudNorm_VST] LEAVE <<< processReplacing()");
		return;
	}

	DEBUG_LOG("InputSamples/OutputSamples: %llu/%llu --> %llu", int64_t(sampleFrames), outputSamples, int64_t(sampleFrames) - outputSamples);
	writeOutputSamplesFlt(outputs, sampleFrames, outputSamples);

	DEBUG_LOG("[DynAudNorm_VST] LEAVE <<< processReplacing()");
}

void DynamicAudioNormalizerVST::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
	DEBUG_LOG("[DynAudNorm_VST] ENTER >>> processDoubleReplacing()");

	if(sampleFrames < 1)
	{
		return; /*number of samples is zero or even negative!*/
	}

	if(!p->instance)
	{
		outputMessage("[DynAudNorm_VST] ISANITY: Host called processReplacing() *before* resume() has been called -> workaround actived!");
		resume();
	}

	updateBufferSize(sampleFrames);
	readInputSamplesDbl(inputs, sampleFrames);

	int64_t outputSamples;
	if(!p->instance->processInplace(p->temp, sampleFrames, outputSamples))
	{
		showErrorMsg("Dynamic Audio Normalizer processing failed!");
		DEBUG_LOG("[DynAudNorm_VST] LEAVE <<< processDoubleReplacing()");
		return;
	}

	writeOutputSamplesDbl(outputs, sampleFrames, outputSamples);

	DEBUG_LOG("[DynAudNorm_VST] LEAVE <<< processDoubleReplacing()");
}

VstInt32 DynamicAudioNormalizerVST::getGetTailSize(void)
{
	outputMessage("[DynAudNorm_VST] DynamicAudioNormalizerVST::getGetTailSize()");

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
		outputMessage("[DynAudNorm_VST] WRN: Increasing internal buffer size: %u -> %u", ((unsigned int)p->tempSize), ((unsigned int)requiredSize));
		for(size_t i = 0; i < 2; i++)
		{
			if(p->tempSize > 0)
			{
				delete [] (p->temp[i]);
			}
			try
			{
				p->temp[i] = allocBuffer<double>(requiredSize);
			}
			catch(...)
			{
				outputMessage("[DynAudNorm_VST] ERR: Allocation of size %u failed!", (unsigned int)(sizeof(double) * requiredSize));
				showErrorMsg("Memory allocation has failed: Out of memory!");
				_exit(0xC0000017);
			}
		}
		p->tempSize = requiredSize;
	}
}

void DynamicAudioNormalizerVST::readInputSamplesFlt(const float *const *const inputs, const int64_t sampleCount)
{
	for(size_t c = 0; c < 2; c++)
	{
		for(int64_t i = 0; i < sampleCount; i++)
		{
			p->temp[c][i] = inputs[c][i];
		}
	}
}

void DynamicAudioNormalizerVST::readInputSamplesDbl(const double *const *const inputs, const int64_t sampleCount)
{
	for(size_t c = 0; c < 2; c++)
	{
		memcpy(&p->temp[c][0], &inputs[c][0], sizeof(double) * size_t(sampleCount));
	}
}

void DynamicAudioNormalizerVST::writeOutputSamplesFlt(float *const *const outputs, const int64_t requiredSamples, const int64_t availableSamples)
{
	if(availableSamples == 0)
	{
		for(size_t c = 0; c < 2; c++)
		{
			memset(&outputs[c][0], 0, sizeof(float) * size_t(requiredSamples));
		}
	}
	else if(availableSamples < requiredSamples)
	{
		const int64_t offset = requiredSamples - availableSamples;
		for(size_t c = 0; c < 2; c++)
		{
			memset(&outputs[c][0], 0, sizeof(float) * size_t(offset));
			float *outPtr = &outputs[c][offset];
			for(int64_t i = 0; i < availableSamples; i++)
			{
				*(outPtr++) = static_cast<float>(p->temp[c][i]);
			}
		}
	}
	else if(availableSamples == requiredSamples)
	{
		for(size_t c = 0; c < 2; c++)
		{
			for(int64_t i = 0; i < availableSamples; i++)
			{
				outputs[c][i] = static_cast<float>(p->temp[c][i]);
			}
		}
	}
	else
	{
		showErrorMsg("Number of output samples exceeds output buffer size!");
	}
}

void DynamicAudioNormalizerVST::writeOutputSamplesDbl(double *const *const outputs, const int64_t requiredSamples, const int64_t availableSamples)
{
	if(availableSamples == 0)
	{
		for(size_t c = 0; c < 2; c++)
		{
			memset(&outputs[c][0], 0, sizeof(double) * size_t(requiredSamples));
		}
	}
	else if(availableSamples < requiredSamples)
	{
		const int64_t offset = requiredSamples - availableSamples;
		for(size_t c = 0; c < 2; c++)
		{
			memset(&outputs[c][0], 0, sizeof(double) * size_t(offset));
			memcpy(&outputs[c][offset], &p->temp[c][0], sizeof(double) * size_t(availableSamples));
		}
	}
	else if(availableSamples == requiredSamples)
	{
		for(size_t c = 0; c < 2; c++)
		{
			memcpy(&outputs[c][0], &p->temp[c][0], sizeof(double) * size_t(availableSamples));
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

static int8_t g_initialized = -1;
static MY_CRITSEC_INIT(g_createEffMutex);

static void appendStr(wchar_t *buffer, const size_t &size, const wchar_t *const text, ...)
{
	wchar_t temp[128];
	va_list argList;
	va_start(argList, text);
	_vsnwprintf_s(temp, 128, _TRUNCATE, text, argList);
	va_end(argList);
	wcsncat_s(buffer, size, temp, _TRUNCATE);
}

static bool showAboutScreen(const uint32_t & major, const uint32_t & minor, const uint32_t & patch, const char *const date, const char *const time, const char *const compiler, const char *const arch, const bool &debug)
{
	wchar_t text[1024] = { '\0' };
	appendStr(text, 1024, L"Dynamic Audio Normalizer, VST Wrapper, Version %u.%02u-%u, %s\n", major, minor, patch, (debug ? L"DEBGU" : L"Release"));
	appendStr(text, 1024, L"Copyright (c) 2014-%S LoRd_MuldeR <mulder2@gmx.de>.\n", &date[7]);
	appendStr(text, 1024, L"Built on %S at %S with %S for %S.\n\n", date, time, compiler, arch);
	appendStr(text, 1024, L"This library is free software; you can redistribute it and/or\n");
	appendStr(text, 1024, L"modify it under the terms of the GNU Lesser General Public\n");
	appendStr(text, 1024, L"License as published by the Free Software Foundation; either\n");
	appendStr(text, 1024, L"version 2.1 of the License, or (at your option) any later version.\n\n");
	appendStr(text, 1024, L"This library is distributed in the hope that it will be useful,\n");
	appendStr(text, 1024, L"but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	appendStr(text, 1024, L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n");
	appendStr(text, 1024, L"Lesser General Public License for more details.\n\n");
	appendStr(text, 1024, L"Please click 'OK' if you agree to the above notice or 'Cancel' otherwise...\n");

	return (MessageBoxW(NULL, text, L"Dynamic Audio Normalizer", MB_SYSTEMMODAL | MB_OKCANCEL | MB_DEFBUTTON2) == IDOK);
}

static bool initializeCoreLibrary(void)
{
	MDynamicAudioNormalizer::setLogFunction(logFunction);

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);
	outputMessage("[DynAudNorm_VST] Dynamic Audio Normalizer VST-Wrapper (v%u.%02u-%u)", major, minor, patch);

	wchar_t appUuid[64];
	if(getAppUuid(appUuid, 64))
	{
		const DWORD version = (1000u * major) + (10u * minor) + patch;
		if(regValueGet(appUuid) != version)
		{
			const char *date, *time, *compiler, *arch; bool debug;
			MDynamicAudioNormalizer::getBuildInfo(&date, &time, &compiler, &arch, debug);

			if(!showAboutScreen(major, minor, patch, date, time, compiler, arch, debug))
			{
				return false;
			}
			regValueSet(appUuid, version);
		}
	}

	return true;
}

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	MY_CRITSEC_ENTER(g_createEffMutex);
	AudioEffect *effectInstance = NULL;

	if(g_initialized < 0)
	{
		g_initialized = initializeCoreLibrary() ? 1 : 0;
	}
	
	if(g_initialized > 0)
	{
		effectInstance = new DynamicAudioNormalizerVST(audioMaster);
	}
	
	MY_CRITSEC_LEAVE(g_createEffMutex);
	return effectInstance;
}
