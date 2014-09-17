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

#ifndef DYNAMICAUDIONORMALIZERVST_H
#define DYNAMICAUDIONORMALIZERVST_H

//VST SDK includes
#include "public.sdk/source/vst2.x/audioeffectx.h"

//Forward declaration
class DynamicAudioNormalizerVST_PrivateData;

//Standard Lib
#include <stdint.h>

class DynamicAudioNormalizerVST : public AudioEffectX
{
public:
	DynamicAudioNormalizerVST(audioMasterCallback audioMaster);
	~DynamicAudioNormalizerVST();

	virtual void open(void);
	virtual void close(void);
	virtual void suspend(void);
	virtual void resume(void);

	// Processing
	virtual void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual void processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames);
	virtual VstInt32 getGetTailSize(void);

	// Program
	virtual void setProgram(VstInt32 program);
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);
	virtual bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);

	// Parameters
	virtual void setParameter(VstInt32 index, float value);
	virtual float getParameter(VstInt32 index);
	virtual float getParameterDefault(VstInt32 index);
	virtual double getParameterValue(VstInt32 index);
	virtual void getParameterLabel(VstInt32 index, char* label);
	virtual void getParameterDisplay(VstInt32 index, char* text);
	virtual void getParameterName(VstInt32 index, char* text);

	// Information
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion(void);
	virtual VstPlugCategory getPlugCategory(void);

private:
	bool createNewInstance(const uint32_t sampleRate);
	void updateBufferSize(const size_t requiredSize);
	void readInputSamplesFlt(const float  *const *const inputs, const int64_t sampleCount);
	void readInputSamplesDbl(const double *const *const inputs, const int64_t sampleCount);
	void writeOutputSamplesFlt(float  *const *const outputs, const int64_t requiredSamples, const int64_t availableSamples);
	void writeOutputSamplesDbl(double *const *const outputs, const int64_t requiredSamples, const int64_t availableSamples);
	void forceUpdateParameters(void);

	DynamicAudioNormalizerVST_PrivateData *const p;
};

#endif //DYNAMICAUDIONORMALIZERVST_H
