//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Microsoft.NET Wrapper
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

#include "DynamicAudioNormalizerNET.h"

#include <stdexcept>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

namespace DynamicAudioNormalizer
{
	public ref struct Error: public System::Exception
	{
	public:
		Error(const char* message) : Exception(gcnew String(message)) {}
	};
}

class PinnedArray2D
{
public:
	PinnedArray2D(array<double,2> ^managedArray)
	{
		if(managedArray->Rank != 2)
		{
			throw gcnew DynamicAudioNormalizer::Error("Managed array is not a 2D array!");
		}

		m_dimOuter = managedArray->GetLength(0);
		m_dimInner = managedArray->GetLength(1);

		m_handle = GCHandle::Alloc(managedArray, GCHandleType::Pinned);
		m_ptr = new double*[m_dimOuter];
		double *basePointer = reinterpret_cast<double*>(m_handle.AddrOfPinnedObject().ToPointer());

		for(size_t d = 0; d < m_dimOuter; d++)
		{
			m_ptr[d] = basePointer;
			basePointer += m_dimInner;
		}
	}

	~PinnedArray2D(void)
	{
		delete [] m_ptr;
		m_handle.Free();
	}

	inline double **data(void)
	{
		return m_ptr;
	}

	inline const size_t &dimOuter(void) const
	{
		return m_dimOuter;
	}

	inline const size_t &dimInner(void) const
	{
		return m_dimInner;
	}

private:
	GCHandle m_handle;
	double **m_ptr;
	size_t m_dimOuter;
	size_t m_dimInner;
};

#define TRY_CATCH(FUNC, ...) do \
{ \
	try \
	{ \
		return p_##FUNC(__VA_ARGS__); \
	} \
	catch(...) \
	{ \
		throw gcnew DynamicAudioNormalizer::Error("Unhandeled exception in native MDynamicAudioNormalizer function!"); \
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
		throw gcnew DynamicAudioNormalizer::Error("Unhandeled exception in native MDynamicAudioNormalizer function!"); \
	} \
} \
while(0)

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
		throw gcnew DynamicAudioNormalizer::Error("Creation of native MDynamicAudioNormalizer instance has failed!");
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
		throw gcnew DynamicAudioNormalizer::Error("Initialization of native MDynamicAudioNormalizer instance has failed!");
	}
	
	if(!initialized)
	{
		throw gcnew DynamicAudioNormalizer::Error("Initialization of native MDynamicAudioNormalizer instance has failed!");
	}
}

DynamicAudioNormalizer::DynamicAudioNormalizerNET::~DynamicAudioNormalizerNET(void)
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

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::processInplace(array<double,2> ^samplesInOut, const int64_t inputSize)
{
	TRY_CATCH(processInplace, samplesInOut, inputSize);
}

int64_t DynamicAudioNormalizer::DynamicAudioNormalizerNET::p_processInplace(array<double,2> ^samplesInOut, const int64_t inputSize)
{
	uint32_t channels, sampleRate, frameLen, filterSize;
	if(!m_instace->getConfiguration(channels, sampleRate, frameLen, filterSize))
	{
		throw gcnew DynamicAudioNormalizer::Error("Failed to retrieve configuration of native MDynamicAudioNormalizer instance!");
	}

	PinnedArray2D samplesInOutPinned(samplesInOut);
	if((samplesInOutPinned.dimOuter() != channels) || (samplesInOutPinned.dimInner() < inputSize))
	{
		throw gcnew DynamicAudioNormalizer::Error("Array dimension mismatch: Array must have dimension CHANNEL_COUNT x INPUT_SIZE!");
	}

	int64_t outputSize = -1;
	if(!m_instace->processInplace(samplesInOutPinned.data(), inputSize, outputSize))
	{
		throw gcnew DynamicAudioNormalizer::Error("Native MDynamicAudioNormalizer instance failed to process samples in-place!");
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
		throw gcnew DynamicAudioNormalizer::Error("Failed to retrieve configuration of native MDynamicAudioNormalizer instance!");
	}

	PinnedArray2D samplesOutPinned(samplesOut);
	if((samplesOutPinned.dimOuter() != channels) || (samplesOutPinned.dimInner() < 1))
	{
		throw gcnew DynamicAudioNormalizer::Error("Array dimension mismatch: Array must have dimension CHANNEL_COUNT x INPUT_SIZE!");
	}

	int64_t outputSize = -1;
	if(!m_instace->flushBuffer(samplesOutPinned.data(), samplesOutPinned.dimInner(), outputSize))
	{
		throw gcnew DynamicAudioNormalizer::Error("Native MDynamicAudioNormalizer instance failed to flush the buffer!");
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
		throw gcnew DynamicAudioNormalizer::Error("Failed to reset the native MDynamicAudioNormalizer instance!");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Information
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
		throw gcnew DynamicAudioNormalizer::Error("Failed to get configuration of native MDynamicAudioNormalizer instance!");
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
		throw gcnew DynamicAudioNormalizer::Error("Failed to get internal delay of MDynamicAudioNormalizer instance!");
	}

	return delayInSamples;
}
