//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Microsoft.NET Wrapper
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "DynamicAudioNormalizerNET.h"

#include <stdexcept>
#include <vector>
#include <Common.h>

using namespace System::Threading;

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

template<class T>
static void INIT_ARRAY_2D(T *basePtr, T **outPtr, const int64_t dimOuter , const int64_t dimInner)
{
	for(int64_t i = 0; i < dimOuter; i++)
	{
		outPtr[i] = basePtr;
		basePtr += dimInner;
	}
}

#define PIN_ARRAY_2D(ARRAY, TYPE) \
	const int64_t ARRAY##_dimOuter = ARRAY->GetLongLength(0); \
	const int64_t ARRAY##_dimInner = ARRAY->GetLongLength(1); \
	TYPE **ARRAY##_ptr = (TYPE**) alloca(sizeof(TYPE*) * ((size_t) ARRAY##_dimOuter)); \
	pin_ptr<TYPE> ARRAY##_pinned = &ARRAY[0,0]; \
	INIT_ARRAY_2D<TYPE>(ARRAY##_pinned, ARRAY##_ptr, ARRAY##_dimOuter, ARRAY##_dimInner)

#define TRY_CATCH(FUNC, ...) do \
{ \
	try \
	{ \
		return p_##FUNC(__VA_ARGS__); \
	} \
	catch(...) \
	{ \
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Unhandeled exception in native MDynamicAudioNormalizer function!"); \
	} \
} \
while(0)

#define TRY_CATCH_NORET(FUNC, ...) do \
{ \
	try \
	{ \
		p_##FUNC(__VA_ARGS__); \
	} \
	catch(...) \
	{ \
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Unhandeled exception in native MDynamicAudioNormalizer function!"); \
	} \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Logging Functions
///////////////////////////////////////////////////////////////////////////////

namespace DynamicAudioNormalizer
{
	private ref class Lock
	{
	public:
		Lock(Object ^ object)
		:
			m_object(object)
		{
			Monitor::Enter(m_object);
		}

		~Lock()
		{
			Monitor::Exit(m_object);
		}

	private:
		Object ^m_object;
	};

	private ref class LoggingSupport
	{
	public:
		static void log(const int &logLevel, const char *const message)
		{
			Lock lock(m_mutex);
			if(m_handler != nullptr)
			{
				try
				{
					m_handler->Invoke(logLevel, gcnew String(message));
				}
				catch(...)
				{
				}
			}
		}
		
		static void setHandler(DynamicAudioNormalizerNET_Logger ^handler)
		{
			Lock lock(m_mutex);
			m_handler = handler;
		}

	private:
		static DynamicAudioNormalizerNET_Logger ^m_handler = nullptr;
		static Object^ m_mutex = gcnew Object();
	};
}

static void dotNetLoggingHandler(const int logLevel, const char *const message)
{
	DynamicAudioNormalizer::LoggingSupport::log(logLevel, message);
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizer::DynamicAudioNormalizerNET::setLogger(DynamicAudioNormalizerNET_Logger ^logger)
{
	LoggingSupport::setHandler(logger);
	MDynamicAudioNormalizer::setLogFunction(dotNetLoggingHandler);
}

void DynamicAudioNormalizer::DynamicAudioNormalizerNET::getVersionInfo([Out] uint32_t %major, [Out] uint32_t %minor, [Out] uint32_t %patch)
{
	pin_ptr<uint32_t> majorPtr = &major;
	pin_ptr<uint32_t> minorPtr = &minor;
	pin_ptr<uint32_t> patchPtr = &patch;

	MDynamicAudioNormalizer::getVersionInfo(*majorPtr, *minorPtr, *patchPtr);
}

void DynamicAudioNormalizer::DynamicAudioNormalizerNET::getBuildInfo([Out] String ^%date, [Out] String ^%time, [Out] String ^%compiler, [Out] String ^%arch, [Out] bool %debug)
{
	const char *datePtr, *timePtr, *compilerPtr, *archPtr; bool debugRef;
	MDynamicAudioNormalizer::getBuildInfo(&datePtr, &timePtr, &compilerPtr, &archPtr, debugRef);

	date = gcnew String(datePtr);
	time = gcnew String(timePtr);
	compiler = gcnew String(compilerPtr);
	arch = gcnew String(archPtr);
	debug = debugRef;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

static MDynamicAudioNormalizer *createNewInstance(const uint32_t &channels, const uint32_t &sampleRate, const uint32_t &frameLenMsec, const uint32_t &filterSize, const double &peakValue, const double &maxAmplification, const double &targetRms, const double &compressFactor, const bool &channelsCoupled, const bool &enableDCCorrection, const bool &altBoundaryMode)
{
	try
	{
		return new MDynamicAudioNormalizer(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode, NULL);
	}
	catch(...)
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Creation of native MDynamicAudioNormalizer instance has failed!");
	}
}

DynamicAudioNormalizer::DynamicAudioNormalizerNET::DynamicAudioNormalizerNET(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const double compressFactor, const bool channelsCoupled, const bool enableDCCorrection, const bool altBoundaryMode)
:
	m_instace(createNewInstance(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode))
{
	bool initialized = false;
	try
	{
		initialized = m_instace->initialize();
	}
	catch(...)
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Initialization of native MDynamicAudioNormalizer instance has failed!");
	}
	
	if(!initialized)
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Initialization of native MDynamicAudioNormalizer instance has failed!");
	}
}

DynamicAudioNormalizer::DynamicAudioNormalizerNET::~DynamicAudioNormalizerNET(void)
{
	this->!DynamicAudioNormalizerNET();
	GC::SuppressFinalize(this);
}

DynamicAudioNormalizer::DynamicAudioNormalizerNET::!DynamicAudioNormalizerNET(void)
{
	try
	{
		delete m_instace;
	}
	catch(...)
	{
		throw gcnew System::Exception("Destruction of native MDynamicAudioNormalizer instance has failed!");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Processing Functions
///////////////////////////////////////////////////////////////////////////////

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::process(array<double, 2> ^samplesIn, array<double, 2> ^samplesOut, const int64_t inputSize)
{
	TRY_CATCH(process, samplesIn, samplesOut, inputSize);
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::p_process(array<double, 2> ^samplesIn, array<double, 2> ^samplesOut, const int64_t inputSize)
{
	uint32_t channels, sampleRate, frameLen, filterSize;
	if (!m_instace->getConfiguration(channels, sampleRate, frameLen, filterSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Failed to retrieve configuration of native MDynamicAudioNormalizer instance!");
	}

	PIN_ARRAY_2D(samplesIn, double);
	if ((samplesIn_dimOuter != channels) || (samplesIn_dimInner < inputSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Array dimension mismatch: Array must have dimension CHANNEL_COUNT x INPUT_SIZE!");
	}

	PIN_ARRAY_2D(samplesOut, double);
	if ((samplesOut_dimOuter != channels) || (samplesOut_dimInner < inputSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Array dimension mismatch: Array must have dimension CHANNEL_COUNT x INPUT_SIZE!");
	}

	int64_t outputSize = -1;
	if (!m_instace->process(samplesIn_ptr, samplesOut_ptr, inputSize, outputSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Native MDynamicAudioNormalizer instance failed to process samples!");
	}

	return outputSize;
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::processInplace(array<double,2> ^samplesInOut, const int64_t inputSize)
{
	TRY_CATCH(processInplace, samplesInOut, inputSize);
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::p_processInplace(array<double,2> ^samplesInOut, const int64_t inputSize)
{
	uint32_t channels, sampleRate, frameLen, filterSize;
	if(!m_instace->getConfiguration(channels, sampleRate, frameLen, filterSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Failed to retrieve configuration of native MDynamicAudioNormalizer instance!");
	}

	PIN_ARRAY_2D(samplesInOut, double);
	if((samplesInOut_dimOuter != channels) || (samplesInOut_dimInner < inputSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Array dimension mismatch: Array must have dimension CHANNEL_COUNT x INPUT_SIZE!");
	}

	int64_t outputSize = -1;
	if(!m_instace->processInplace(samplesInOut_ptr, inputSize, outputSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Native MDynamicAudioNormalizer instance failed to process samples in-place!");
	}

	return outputSize;
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::flushBuffer(array<double,2> ^samplesInOut)
{
	TRY_CATCH(flushBuffer, samplesInOut);
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::p_flushBuffer(array<double,2> ^samplesOut)
{
	uint32_t channels, sampleRate, frameLen, filterSize;
	if(!m_instace->getConfiguration(channels, sampleRate, frameLen, filterSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Failed to retrieve configuration of native MDynamicAudioNormalizer instance!");
	}

	PIN_ARRAY_2D(samplesOut, double);
	if((samplesOut_dimOuter != channels) || (samplesOut_dimInner < 1))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Array dimension mismatch: Array must have dimension CHANNEL_COUNT x INPUT_SIZE!");
	}

	int64_t outputSize = -1;
	if(!m_instace->flushBuffer(samplesOut_ptr, samplesOut_dimInner, outputSize))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Native MDynamicAudioNormalizer instance failed to flush the buffer!");
	}

	return outputSize;
}

void DynamicAudioNormalizer::DynamicAudioNormalizerNET::reset(void)
{
	TRY_CATCH_NORET(reset);
}

void DynamicAudioNormalizer::DynamicAudioNormalizerNET::p_reset(void)
{
	if(!m_instace->reset())
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Failed to reset the native MDynamicAudioNormalizer instance!");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Other Functions
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizer::DynamicAudioNormalizerNET::getConfiguration([Out] uint32_t %channels, [Out] uint32_t %sampleRate, [Out] uint32_t %frameLen, [Out] uint32_t %filterSize)
{
	TRY_CATCH_NORET(getConfiguration, channels, sampleRate, frameLen, filterSize);
}

void DynamicAudioNormalizer::DynamicAudioNormalizerNET::p_getConfiguration([Out] uint32_t %channels, [Out] uint32_t %sampleRate, [Out] uint32_t %frameLen, [Out] uint32_t %filterSize)
{
	pin_ptr<uint32_t> channelsPtr   = &channels;
	pin_ptr<uint32_t> sampleRatePtr = &sampleRate;
	pin_ptr<uint32_t> frameLenPtr   = &frameLen;
	pin_ptr<uint32_t> filterSizePtr = &filterSize;

	if(!m_instace->getConfiguration(*channelsPtr, *sampleRatePtr, *frameLenPtr, *filterSizePtr))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Failed to get configuration of native MDynamicAudioNormalizer instance!");
	}
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::getInternalDelay(void)
{
	TRY_CATCH(getInternalDelay);
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::p_getInternalDelay(void)
{
	int64_t delayInSamples;

	if(!m_instace->getInternalDelay(delayInSamples))
	{
		throw gcnew DynamicAudioNormalizer::DynamicAudioNormalizerNET_Error("Failed to get internal delay of MDynamicAudioNormalizer instance!");
	}

	return delayInSamples;
}
