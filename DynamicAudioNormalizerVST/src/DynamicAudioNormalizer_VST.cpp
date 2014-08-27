//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
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

#include "DynamicAudioNormalizer_VST.h"

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

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

static void showErrorMsg(const char *const text)
{
	MessageBoxA(NULL, text, "Dynamic Audio Normalizer", MB_ICONSTOP | MB_TOPMOST);
}

///////////////////////////////////////////////////////////////////////////////
// Private Data
///////////////////////////////////////////////////////////////////////////////

class DynamicAudioNormalizerVST_PrivateData
{
public:
	MDynamicAudioNormalizer *instance;
	double *temp[2];
	uint32_t tempSize;
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

DynamicAudioNormalizerVST::DynamicAudioNormalizerVST(audioMasterCallback audioMaster)
:
	AudioEffectX (audioMaster, 1, 1),
	p(new DynamicAudioNormalizerVST_PrivateData())
{
	static const VstInt32 uniqueId = CCONST('!','c','2','N');

	setNumInputs(2);			// stereo in
	setNumOutputs(2);			// stereo out
	setUniqueID(uniqueId);		// identify
	canProcessReplacing();		// supports replacing output
	canDoubleReplacing();		// supports double precision processing
	noTail(false);				// does have Tail!

	vst_strncpy (programName, "Default", kVstMaxProgNameLen);
	
	p->instance = 0;
	p->temp[0] = p->temp[1] = NULL;
	p->tempSize = 0;
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
	}

	delete p;
}

///////////////////////////////////////////////////////////////////////////////
// Program Handling
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::setProgramName (char* name)
{
	vst_strncpy (programName, name, kVstMaxProgNameLen);
}

void DynamicAudioNormalizerVST::getProgramName (char* name)
{
	vst_strncpy (name, programName, kVstMaxProgNameLen);
}

void DynamicAudioNormalizerVST::setParameter (VstInt32 index, float value)
{
	fDummyParam = value;
}

float DynamicAudioNormalizerVST::getParameter (VstInt32 index)
{
	return fDummyParam;
}

void DynamicAudioNormalizerVST::getParameterName (VstInt32 index, char* label)
{
	vst_strncpy (label, "Dummy", kVstMaxParamStrLen);
}

void DynamicAudioNormalizerVST::getParameterDisplay (VstInt32 index, char* text)
{
	dB2string (fDummyParam, text, kVstMaxParamStrLen);
}

void DynamicAudioNormalizerVST::getParameterLabel (VstInt32 index, char* label)
{
	vst_strncpy (label, "dB", kVstMaxParamStrLen);
}

///////////////////////////////////////////////////////////////////////////////
// Effect Info
///////////////////////////////////////////////////////////////////////////////

bool DynamicAudioNormalizerVST::getEffectName (char* name)
{
	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);

	_snprintf(name, kVstMaxEffectNameLen, "DynamicAudioNormalizer %u.%02u-%u", major, minor, patch);
	name[kVstMaxEffectNameLen-1] = '\0';

	return true;
}

bool DynamicAudioNormalizerVST::getProductString (char* text)
{
	uint32_t major, minor, patch;
	MDynamicAudioNormalizer::getVersionInfo(major, minor, patch);

	_snprintf(text, kVstMaxProductStrLen, "DynamicAudioNormalizer_%u", (100U * major) + minor);
	text[kVstMaxProductStrLen-1] = '\0';

	return true;
}

bool DynamicAudioNormalizerVST::getVendorString (char* text)
{
	vst_strncpy (text, "LoRd_MuldeR <mulder2@gmxde>", kVstMaxVendorStrLen);
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
	/*nothing to do here*/
}

void DynamicAudioNormalizerVST::close(void)
{
	/*nothing to do here*/
}

void DynamicAudioNormalizerVST::resume(void)
{
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
	
	try
	{
		p->instance = new MDynamicAudioNormalizer(2, sampleRate);
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

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	return new DynamicAudioNormalizerVST(audioMaster);
}

