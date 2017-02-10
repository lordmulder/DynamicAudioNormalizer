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

#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

#include <DynamicAudioNormalizer.h>

namespace DynamicAudioNormalizer
{
	public delegate void DynamicAudioNormalizerNET_Logger(int, String^);

	public ref struct DynamicAudioNormalizerNET_Error: public System::Exception
	{
	public:
		DynamicAudioNormalizerNET_Error(const char* message) : Exception(gcnew String(message)) {}
	};

	public ref class DynamicAudioNormalizerNET
	{
	public:
		DynamicAudioNormalizerNET(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const double compressFactor, const bool channelsCoupled, const bool enableDCCorrection, const bool altBoundaryMode);
		~DynamicAudioNormalizerNET(void);
		!DynamicAudioNormalizerNET(void);

		//Static
		static void getVersionInfo([Out] uint32_t %major, [Out] uint32_t %minor, [Out] uint32_t %patch);
		static void getBuildInfo([Out] String ^%date, [Out] String ^%time, [Out] String ^%compiler, [Out] String ^%arch, [Out] bool %debug);
		static void setLogger(DynamicAudioNormalizerNET_Logger ^test);

		//Processing
		int64_t process(array<double, 2> ^samplesIn, array<double, 2> ^samplesOut, const int64_t inputSize);
		int64_t processInplace(array<double,2> ^samplesInOut, const int64_t inputSize);
		int64_t flushBuffer(array<double,2> ^samplesInOut);
		void reset(void);
		
		//Other
		void getConfiguration([Out] uint32_t %channels, [Out] uint32_t %sampleRate, [Out] uint32_t %frameLen, [Out] uint32_t %filterSize);
		int64_t getInternalDelay(void);

	private:
		int64_t p_process(array<double, 2> ^samplesIn, array<double, 2> ^samplesOut, const int64_t inputSize);
		int64_t p_processInplace(array<double,2> ^samplesInOut, const int64_t inputSize);
		int64_t p_flushBuffer(array<double,2> ^samplesInOut);
		void p_reset(void);

		void p_getConfiguration([Out] uint32_t %channels, [Out] uint32_t %sampleRate, [Out] uint32_t %frameLen, [Out] uint32_t %filterSize);
		int64_t p_getInternalDelay(void);

		static DynamicAudioNormalizerNET_Logger ^m_logger;
		MDynamicAudioNormalizer *const m_instace;
	};
}
