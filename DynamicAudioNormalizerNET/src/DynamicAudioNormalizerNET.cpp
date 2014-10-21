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

DynamicAudioNormalizerNET::CDynamicAudioNormalizer::CDynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const double compressFactor, const bool channelsCoupled, const bool enableDCCorrection, const bool altBoundaryMode)
:
	m_instace(new MDynamicAudioNormalizer(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode, NULL))
{
	if(!m_instace->initialize())
	{
		throw std::runtime_error("Initialization of native MDynamicAudioNormalizer instance has failed!");
	}
}

DynamicAudioNormalizerNET::CDynamicAudioNormalizer::~CDynamicAudioNormalizer(void)
{
	delete m_instace;
}

bool DynamicAudioNormalizerNET::CDynamicAudioNormalizer::processInplace(double **samplesInOut, const int64_t inputSize, int64_t &outputSize)
{
	return m_instace->processInplace(samplesInOut, inputSize, outputSize);
}
