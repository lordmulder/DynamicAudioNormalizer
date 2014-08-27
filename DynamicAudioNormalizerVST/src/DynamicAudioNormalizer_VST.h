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
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);

	// Parameters
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);

	// Information
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion ();

private:
	bool createNewInstance(const uint32_t sampleRate);
	void updateBufferSize(const size_t requiredSize);
	void readInputSamplesFlt(const float  *const *const inputs, const int64_t sampleCount);
	void readInputSamplesDbl(const double *const *const inputs, const int64_t sampleCount);
	void writeOutputSamplesFlt(float  *const *const outputs, const int64_t sampleCount, const int64_t outputSamples);
	void writeOutputSamplesDbl(double *const *const outputs, const int64_t sampleCount, const int64_t outputSamples);

	DynamicAudioNormalizerVST_PrivateData *const p;
	float fDummyParam;
	char programName[kVstMaxProgNameLen + 1];
};

#endif //DYNAMICAUDIONORMALIZERVST_H
