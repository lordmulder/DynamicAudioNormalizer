//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Winamp DSP Wrapper
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

//Stdlib
#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <algorithm>

//Win32
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//Winamp
#include <winamp_dsp.h>
#include <winamp_ipc.h>

//Internal
#include <Common.h>

//Dynamic Audio Normalizer API
#include <DynamicAudioNormalizer.h>

//PThread
#if defined(_WIN32) && defined(_MT)
#define PTW32_STATIC_LIB 1
#endif
#include <pthread.h>

//DLL exports
#define DLL_EXPORT __declspec(dllexport)

//Reg key
static const wchar_t *REGISTRY_PATH = L"Software\\MuldeR\\DynAudNorm\\WA5";
static const wchar_t *REGISTRY_NAME = L"Version";

//Critical Section
static char g_loggingBuffer[1024];
static pthread_mutex_t g_loggingMutex = PTHREAD_MUTEX_INITIALIZER;

//Forward declarations
static void config(struct winampDSPModule*);
static int init(struct winampDSPModule*);
static void quit(struct winampDSPModule*);
static int modify_samples(struct winampDSPModule*, short int*, int, int, int, int);
static winampDSPModule *getModule(int);
static int sf(int);
static bool showAboutScreen(const uint32_t&, const uint32_t&, const uint32_t&, const char *const, const char *const, const char *const, const char *const, const bool&, const bool &config);

//Const
static const size_t MAX_CHANNELS = 12;

///////////////////////////////////////////////////////////////////////////////
// Global Data
///////////////////////////////////////////////////////////////////////////////

static char g_description[256] = { '\0' };

static winampDSPHeader g_header =
{
	DSP_HDRVER + 1,
	"Dynamic Audio Normalizer [" __DATE__ "]",
	getModule,
	sf
};

static winampDSPModule g_module =
{
	g_description,
	NULL,	// hwndParent
	NULL,	// hDllInstance
	config,
	init,
	modify_samples,
	quit
};

static struct
{
	uint32_t sampleRate;
	uint32_t bitsPerSample;
	uint32_t channels;
}
g_properties =
{
	44100, 16, 2
};

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
		outputMessage("[DynAudNorm_WA5] NFO: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_WRN:
		outputMessage("[DynAudNorm_WA5] WRN: %s\n", message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_ERR:
		outputMessage("[DynAudNorm_WA5] ERR: %s\n", message);
		break;
	default:
		outputMessage("[DynAudNorm_WA5] DBG: %s\n", message);
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

static double *allocBuffer(size_t size)
{
	double *buffer = NULL;
	try
	{
		buffer = new double[size];
	}
	catch(...)
	{
		outputMessage("[DynAudNorm_WA5] ERR: Allocation of size %u failed!", static_cast<unsigned int>(sizeof(double) * size));
		showErrorMsg("Memory allocation has failed: Out of memory!");
		_exit(0xC0000017);
	}
	return buffer;
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

static MDynamicAudioNormalizer *g_instance = NULL;
static size_t g_sampleBufferSize = 0;
static double *g_sampleBuffer[MAX_CHANNELS];
static long g_lastPosition = -1;

static void updateBufferSize(const int &numsamples)
{
	if(size_t(numsamples) > g_sampleBufferSize)
	{
		if(g_sampleBufferSize > 0)
		{
			for(int c = 0; c < MAX_CHANNELS; c++)
			{
				delete [] (g_sampleBuffer[c]);
				g_sampleBuffer[c] = NULL;
			}
		}
		for(int c = 0; c < MAX_CHANNELS; c++)
		{
			g_sampleBuffer[c] = allocBuffer(numsamples);
		}
		g_sampleBufferSize = numsamples;
	}
}

static void deinterleave(const short int *const input, const int &numsamples, const int &bps, const int &nch)
{
	updateBufferSize(numsamples);

	if(bps == 16)
	{
		const short int *ptr = input;
		for(int i = 0; i < numsamples; i++)
		{
			for(int c = 0; c < nch; c++)
			{
				g_sampleBuffer[c][i] = std::min(1.0, std::max(-1.0, (double(*(ptr++)) / double(SHRT_MAX))));
			}
		}
	}
	else if(bps == 8)
	{
		const int8_t *ptr = reinterpret_cast<const int8_t*>(input);
		for(int i = 0; i < numsamples; i++)
		{
			for(int c = 0; c < nch; c++)
			{
				g_sampleBuffer[c][i] = std::min(1.0, std::max(-1.0, (double(*(ptr++)) / double(INT8_MAX))));
			}
		}
	}
	else
	{
		showErrorMsg("Unsupported bit-depth detected!");
	}
}

static void interleave(short int *const output, const int &numsamples, const int &bps, const int &nch)
{
	updateBufferSize(numsamples);

	if(bps == 16)
	{
		short int *ptr = output;
		for(int i = 0; i < numsamples; i++)
		{
			for(int c = 0; c < nch; c++)
			{
				*(ptr++) = static_cast<short int>(std::max(double(SHRT_MIN), std::min(double(SHRT_MAX), round(g_sampleBuffer[c][i] * double(SHRT_MAX)))));
			}
		}
	}
	else if(bps == 8)
	{
		int8_t *ptr = reinterpret_cast<int8_t*>(output);
		for(int i = 0; i < numsamples; i++)
		{
			for(int c = 0; c < nch; c++)
			{
				*(ptr++) = static_cast<int8_t>(std::max(double(INT8_MIN), std::min(double(INT8_MAX), round(g_sampleBuffer[c][i] * double(INT8_MAX)))));
			}
		}
	}
	else
	{
		showErrorMsg("Unsupported bit-depth detected!");
	}
}

static bool createNewInstance(const uint32_t sampleRate, const uint32_t channelCount)
{
	if(g_instance)
	{
		delete g_instance;
		g_instance = NULL;
	}

	try
	{
		g_instance = new MDynamicAudioNormalizer
		(
			channelCount,
			sampleRate
		);
	}
	catch(...)
	{
		showErrorMsg("Failed to create Dynamic Audio Normalizer instance!");
		return false;
	}

	if(!g_instance->initialize())
	{
		showErrorMsg("Dynamic Audio Normalizer initialization has failed!");
		return false;
	}

	g_lastPosition = -1;
	return true;
}

static bool detectWinampVersion(const HWND &hwndParent)
{
	unsigned long winampVersion      = 0;
	unsigned long winampVersionMajor = 0;
	unsigned long winampVersionMinor = 0;

	if(SendMessageTimeout(hwndParent, WM_WA_IPC, 0, IPC_GETVERSION, SMTO_ABORTIFHUNG, 5000, &winampVersion))
	{
		if((winampVersion >= 0x5000) && (winampVersion <= 0xFFFFF))
		{
			winampVersionMajor = (winampVersion >> 0xC);
			winampVersionMinor = (winampVersion & 0xFF);
		}
	}
	else
	{
		winampVersion = 0xFFFFFFFF;
	}

	outputMessage("[DynAudNorm_WA5] Winamp version is: 0x%X (v%X.%02X)", winampVersion, winampVersionMajor, winampVersionMinor);

	if(winampVersionMajor < 1)
	{
		showErrorMsg("Failed to detect Winamp version. Please use Winamp 5.0 or newer!");
		return false;
	}
	
	if(winampVersionMajor < 5)
	{
		showErrorMsg("Your Winamp version is too old. Please use Winamp 5.0 or newer!");
		return false;
	}

	return true;
}

static bool checkTimestampContinuity(const HWND &hwndParent)
{
	bool discontinuityDetected = false;
	long int currentPos;
	if(SendMessageTimeout(hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME, SMTO_ABORTIFHUNG, 150, reinterpret_cast<unsigned long*>(&currentPos)))
	{
		if(currentPos >= 0)
		{
			if(g_lastPosition >= 0)
			{
				if((currentPos > g_lastPosition) && ((currentPos - g_lastPosition) > 512))
				{
					discontinuityDetected = true;
				}
				else if((currentPos < g_lastPosition) && ((g_lastPosition - currentPos) > 32))
				{
					discontinuityDetected = true;
				}
			}
			g_lastPosition = currentPos;
		}
	}
	return (!discontinuityDetected);
}

///////////////////////////////////////////////////////////////////////////////
// Public Functions
///////////////////////////////////////////////////////////////////////////////

static void config(struct winampDSPModule *this_mod)
{
	outputMessage("[DynAudNorm_WA5] WinampDSP::config(%p)", this_mod);

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);

	const char *date, *time, *compiler, *arch; bool debug;
	MDynamicAudioNormalizer::getBuildInfo(&date, &time, &compiler, &arch, debug);

	showAboutScreen(major, minor, patch, date, time, compiler, arch, debug, true);
}

static int init(struct winampDSPModule *this_mod)
{
	outputMessage("[DynAudNorm_WA5] WinampDSP::init(%p)", this_mod);
	detectWinampVersion(this_mod->hwndParent);

	if(g_sampleBufferSize == 0)
	{
		for(int c = 0; c < MAX_CHANNELS; c++)
		{
			g_sampleBuffer[c] = allocBuffer(1024);
		}
		g_sampleBufferSize = 1024;
	}

	if(!createNewInstance(g_properties.sampleRate, g_properties.channels))
	{
		return 1;
	}

	return 0;
}

static void quit(struct winampDSPModule *this_mod)
{
	outputMessage("[DynAudNorm_WA5] WinampDSP::quit(%p)", this_mod);

	if(g_sampleBufferSize > 0)
	{
		for(int c = 0; c < MAX_CHANNELS; c++)
		{
			delete [] (g_sampleBuffer[c]);
			g_sampleBuffer[c] = NULL;
		}
		g_sampleBufferSize = 0;
	}
}

static int modify_samples(struct winampDSPModule *this_mod, short int *samples, int numsamples, int bps, int nch, int srate)
{
	if(numsamples < 1)
	{
		outputMessage("[DynAudNorm_WA5] Sample count is zero or negative -> nothing to do!");
		return 0;
	}

	if(nch > MAX_CHANNELS)
	{
		showErrorMsg("Maximum channel count has been exceeded!");
		return 0;
	}

	if((g_properties.bitsPerSample != bps) || (g_properties.channels != nch) || (g_properties.sampleRate != srate))
	{
		outputMessage("[DynAudNorm_WA5] WinampDSP::modify_samples(mod=%p, num=%d, bps=%d, nch=%d, srate=%d)", this_mod, numsamples, bps, nch, srate);

		g_properties.bitsPerSample = bps;
		g_properties.channels = nch;
		g_properties.sampleRate = srate;

		if(!createNewInstance(g_properties.sampleRate, g_properties.channels))
		{
			return 0; /*creating the new instance failed!*/
		}
	}
	else if(!checkTimestampContinuity(this_mod->hwndParent))
	{
		outputMessage("[DynAudNorm_WA5] SEEK detected -> resetting!");
		g_instance->reset();
	}

	deinterleave(samples, numsamples, bps, nch);

	int64_t outputSize;
	g_instance->processInplace(g_sampleBuffer, numsamples, outputSize);

	interleave(samples, static_cast<int>(outputSize), bps, nch);
	return static_cast<int>(outputSize);
}

///////////////////////////////////////////////////////////////////////////////
// Winamp DSP API
///////////////////////////////////////////////////////////////////////////////

static int8_t g_initialized = -1;
static pthread_mutex_t g_createEffMutex = PTHREAD_MUTEX_INITIALIZER;

static void appendStr(wchar_t *buffer, const size_t &size, const wchar_t *const text, ...)
{
	wchar_t temp[128];
	va_list argList;
	va_start(argList, text);
	_vsnwprintf_s(temp, 128, _TRUNCATE, text, argList);
	va_end(argList);
	wcsncat_s(buffer, size, temp, _TRUNCATE);
}

static bool showAboutScreen(const uint32_t &major, const uint32_t &minor, const uint32_t &patch, const char *const date, const char *const time, const char *const compiler, const char *const arch, const bool &debug, const bool &config)
{
	wchar_t text[1024] = { '\0' };
	appendStr(text, 1024, L"Dynamic Audio Normalizer, Winamp Wrapper, Version %u.%02u-%u, %s\n", major, minor, patch, (debug ? L"DEBGU" : L"Release"));
	appendStr(text, 1024, L"Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.\n");
	appendStr(text, 1024, L"Built on %S at %S with %S for %S.\n\n", date, time, compiler, arch);
	appendStr(text, 1024, L"This library is free software; you can redistribute it and/or\n");
	appendStr(text, 1024, L"modify it under the terms of the GNU Lesser General Public\n");
	appendStr(text, 1024, L"License as published by the Free Software Foundation; either\n");
	appendStr(text, 1024, L"version 2.1 of the License, or (at your option) any later version.\n\n");
	appendStr(text, 1024, L"This library is distributed in the hope that it will be useful,\n");
	appendStr(text, 1024, L"but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	appendStr(text, 1024, L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n");
	appendStr(text, 1024, L"Lesser General Public License for more details.\n");

	if(config)
	{
		MessageBoxW(NULL, text, L"Dynamic Audio Normalizer", MB_TOPMOST | MB_TASKMODAL | MB_OK);
		return true;
	}
	else
	{
		appendStr(text, 1024, L"\nPlease click 'OK' if you agree to the above notice or 'Cancel' otherwise...\n");
		const int ret = MessageBoxW(NULL, text, L"Dynamic Audio Normalizer", MB_TOPMOST | MB_TASKMODAL | MB_OKCANCEL | MB_DEFBUTTON2);
		return (ret == IDOK);
	}
}

static bool initializeCoreLibrary(void)
{
	MDynamicAudioNormalizer::setLogFunction(logFunction);

	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);

	outputMessage("[DynAudNorm_WA5] Dynamic Audio Normalizer Winamp-Wrapper (v%u.%02u-%u)", major, minor, patch);
	_snprintf_s(g_description, 256, _TRUNCATE, "Dynamic Audio Normalizer v%u.%02u-%u [by LoRd_MuldeR]", major, minor, patch);

	const DWORD version = (1000u * major) + (10u * minor) + patch;
	if(regValueGet(REGISTRY_NAME) != version)
	{
		const char *date, *time, *compiler, *arch; bool debug;
		MDynamicAudioNormalizer::getBuildInfo(&date, &time, &compiler, &arch, debug);

		if(!showAboutScreen(major, minor, patch, date, time, compiler, arch, debug, false))
		{
			return false;
		}
		regValueSet(REGISTRY_NAME, version);
	}

	return true;
}

static winampDSPModule *getModule(int which)
{
	outputMessage("[DynAudNorm_WA5] WinampDSP::getModule(%d)", which);

	winampDSPModule *module = NULL;
	MY_CRITSEC_ENTER(g_createEffMutex);

	if(which == 0)
	{
		if(g_initialized < 0)
		{
			g_initialized = initializeCoreLibrary() ? 1 : 0;
		}
		if(g_initialized > 0)
		{
			module = &g_module;
		}
	}

	MY_CRITSEC_LEAVE(g_createEffMutex);
	return module;
}

static int sf(int v)	/*Note: The "sf" function was copied 1:1 from the example code, I have NO clue what it does!*/
{
	int res;
	res = v * (unsigned long)1103515245;
	res += (unsigned long)13293;
	res &= (unsigned long)0x7FFFFFFF;
	res ^= v;
	return res;
}

extern "C" DLL_EXPORT winampDSPHeader *winampDSPGetHeader2()
{
	outputMessage("[DynAudNorm_WA5] WinampDSP::winampDSPGetHeader2()");
	return &g_header;
}
