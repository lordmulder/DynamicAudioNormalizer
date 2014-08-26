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

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

DynamicAudioNormalizerVST::DynamicAudioNormalizerVST (audioMasterCallback audioMaster)
:
	AudioEffectX (audioMaster, 1, 1)
{
	setNumInputs (2);		// stereo in
	setNumOutputs (2);		// stereo out
	setUniqueID ('Gain');	// identify
	canProcessReplacing ();	// supports replacing output
	canDoubleReplacing ();	// supports double precision processing

	vst_strncpy (programName, "Default", kVstMaxProgNameLen);
}

DynamicAudioNormalizerVST::~DynamicAudioNormalizerVST ()
{
	// nothing to do here
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
	fGain = value;
}

float DynamicAudioNormalizerVST::getParameter (VstInt32 index)
{
	return fGain;
}

void DynamicAudioNormalizerVST::getParameterName (VstInt32 index, char* label)
{
	vst_strncpy (label, "Gain", kVstMaxParamStrLen);
}

void DynamicAudioNormalizerVST::getParameterDisplay (VstInt32 index, char* text)
{
	dB2string (fGain, text, kVstMaxParamStrLen);
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
	return 1000; 
}

///////////////////////////////////////////////////////////////////////////////
// Audio Processing
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizerVST::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* in1  =  inputs[0];
	float* in2  =  inputs[1];
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	while (--sampleFrames >= 0)
	{
		(*out1++) = (*in1++) * fGain;
		(*out2++) = (*in2++) * fGain;
	}
}

void DynamicAudioNormalizerVST::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
	double* in1  =  inputs[0];
	double* in2  =  inputs[1];
	double* out1 = outputs[0];
	double* out2 = outputs[1];
	double dGain = fGain;

	while (--sampleFrames >= 0)
	{
		(*out1++) = (*in1++) * dGain;
		(*out2++) = (*in2++) * dGain;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Create Instance
///////////////////////////////////////////////////////////////////////////////

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	return new DynamicAudioNormalizerVST(audioMaster);
}

