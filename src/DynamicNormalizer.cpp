///////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#include "DynamicNormalizer.h"

#include <cmath>
#include <algorithm>
#include <cassert>
#include <deque>

static inline double UPDATE_VALUE(const double &NEW, const double &OLD, const double &aggressiveness)
{
	assert((aggressiveness >= 0.0) && (aggressiveness <= 1.0));
	return (aggressiveness * NEW) + ((1.0 - aggressiveness) * OLD);
}

///////////////////////////////////////////////////////////////////////////////
// DynamicNormalizer_Private
///////////////////////////////////////////////////////////////////////////////

class DynamicNormalizer::PrivateData
{
public:
	PrivateData(void);

	int currentPass;
	std::deque<double> **maxAmplification;
	double **frameBuffer;

	RingBuffer **bufferSrc;
	RingBuffer **bufferOut;
};

DynamicNormalizer::PrivateData::PrivateData(void)
{
	currentPass = DynamicNormalizer::PASS_1ST;
	frameBuffer = NULL;
	bufferSrc = NULL;
	bufferOut = NULL;
	maxAmplification = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

DynamicNormalizer::DynamicNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, FILE *const logFile)
:
	p(new DynamicNormalizer::PrivateData),
	m_channels(channels),
	m_sampleRate(sampleRate),
	m_frameLen(static_cast<uint32_t>(round(double(sampleRate) * (double(frameLenMsec) / 1000.0)))),
	m_channelsCoupled(channelsCoupled),
	m_enableDCCorrection(enableDCCorrection),
	m_peakValue(peakValue),
	m_maxAmplification(maxAmplification),
	m_logFile(logFile)
{
	p->currentPass = PASS_1ST;

	if((m_channels < 1) || (m_channels > 8))
	{
		MY_THROW("Invalid or unsupported channel count. Should be in the 1 to 8 range!");
	}
	if((m_sampleRate < 11025) || (m_channels > 192000))
	{
		MY_THROW("Invalid or unsupported sampling rate. Should be in the 11025 to 192000 range!");
	}
	if((m_frameLen < 32) || (m_frameLen > 2097152))
	{
		MY_THROW("Invalid or unsupported frame size. Should be in the 32 to 2097152 range!");
	}

	p->bufferSrc = new RingBuffer*[channels];
	p->bufferOut = new RingBuffer*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		p->bufferSrc[channel] = new RingBuffer(m_frameLen);
		p->bufferOut[channel] = new RingBuffer(m_frameLen);
	}

	p->frameBuffer = new double*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		p->frameBuffer[channel] = new double[m_frameLen];
	}

	p->maxAmplification = new std::deque<double>*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		p->maxAmplification[channel] = new std::deque<double>();
	}

	LOG_DBG(TXT("---------Parameters---------"));
	LOG_DBG(TXT("Frame size     : %u"),   m_frameLen);
	LOG_DBG(TXT("Channels       : %u"),   m_channels);
	LOG_DBG(TXT("Sampling rate  : %u"),   m_sampleRate);
	LOG_DBG(TXT("Chan. coupling : %s"),   m_channelsCoupled    ? TXT("YES") : TXT("NO"));
	LOG_DBG(TXT("DC correction  : %s"),   m_enableDCCorrection ? TXT("YES") : TXT("NO"));
	LOG_DBG(TXT("Peak value     : %.4f"), m_peakValue);
	LOG_DBG(TXT("Max amp factor : %.4f"), m_maxAmplification);
	LOG_DBG(TXT("---------Parameters---------\n"));
}

DynamicNormalizer::~DynamicNormalizer(void)
{
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		MY_DELETE_ARRAY(p->bufferSrc[channel]);
		MY_DELETE_ARRAY(p->bufferOut[channel]);
		MY_DELETE_ARRAY(p->frameBuffer[channel]);
		MY_DELETE(p->maxAmplification[channel]);
	}

	MY_DELETE_ARRAY(p->bufferSrc);
	MY_DELETE_ARRAY(p->bufferOut);
	MY_DELETE_ARRAY(p->frameBuffer);
	MY_DELETE_ARRAY(p->maxAmplification);
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool DynamicNormalizer::processInplace(double **samples, int64_t inputSize, int64_t &outputSize)
{
	outputSize = 0;

	uint32_t inputPos = 0;
	uint32_t inputSamplesLeft = static_cast<uint32_t>(std::min(std::max(inputSize, 0i64), int64_t(UINT32_MAX)));
	
	uint32_t outputPos = 0;
	uint32_t outputBufferLeft = 0;

	bool bStop = (inputSamplesLeft < 1);

	while(!bStop)
	{
		bStop = true;

		//Read input samples
		while((inputSamplesLeft > 0) && (p->bufferSrc[0]->getFree() > 0))
		{
			const uint32_t copyLen = std::min(inputSamplesLeft, p->bufferSrc[0]->getFree());
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				p->bufferSrc[channel]->append(&samples[channel][inputPos], copyLen);
			}
			inputPos += copyLen;
			inputSamplesLeft -= copyLen;
			outputBufferLeft += copyLen;
			bStop = false;
		}

		//Process frames
		while((p->bufferSrc[0]->getUsed() >= m_frameLen) && (p->bufferOut[0]->getFree() >= m_frameLen))
		{
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				p->bufferSrc[channel]->read(p->frameBuffer[channel], m_frameLen);
			}
			processNextFrame();
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				p->bufferOut[channel]->append(p->frameBuffer[channel], m_frameLen);
			}
			bStop = false;
		}

		//Write output samples
		while((outputBufferLeft > 0) && (p->bufferOut[0]->getUsed() > 0))
		{
			const uint32_t copyLen = std::min(outputBufferLeft, p->bufferOut[0]->getUsed());
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				p->bufferOut[channel]->read(&samples[channel][outputPos], copyLen);
			}
			outputPos += copyLen;
			outputBufferLeft -= copyLen;
			bStop = false;
		}
	}

	outputSize = int64_t(outputPos);

	if(inputSamplesLeft > 0)
	{
		LOG_WRN(TXT("No all input samples could be processed -> discarding pending input!"));
		return false;
	}

	return true;
}

bool DynamicNormalizer::setPass(const int pass)
{
	if((pass != PASS_1ST) && (pass != PASS_2ND))
	{
		LOG_ERR(TXT("Invalid pass value %d specified -> ignoring!"), pass);
		return false;
	}

	//Clear ring buffers
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		p->bufferSrc[channel]->clear();
		p->bufferOut[channel]->clear();
	}

	//Setup the new processing pass
	if(pass == PASS_1ST)
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			p->maxAmplification[channel]->clear();
		}
	}
	else
	{
		if(p->maxAmplification[0]->size() < 1)
		{
			LOG_WRN(TXT("No information from 1st pass stored yet!"));
		}
		else
		{
			//initializeSecondPass();
		}
	}

	//Write to log file
	if(m_logFile)
	{
		writeToLogFile();
	}

	p->currentPass = pass;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Procesing Functions
///////////////////////////////////////////////////////////////////////////////

void DynamicNormalizer::processNextFrame(void)
{
	//DC correction
	//if(m_enableDCCorrection)
	//{
	//	perfromDCCorrection();
	//}
	
	switch(p->currentPass)
	{
	case PASS_1ST:
		analyzeCurrentFrame();
		break;
	case PASS_2ND:
		amplifyCurrentFrame();
		break;
	default:
		MY_THROW("Invalid pass value detected!");
	}
}

void DynamicNormalizer::analyzeCurrentFrame(void)
{
	if(m_channelsCoupled)
	{
		const double peakMagnitude = findPeakMagnitude();
		const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
		
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			p->maxAmplification[channel]->push_back(currentAmplificationFactor);
		}
	}
	else
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			const double peakMagnitude = findPeakMagnitude(channel);
			const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
			p->maxAmplification[channel]->push_back(currentAmplificationFactor);
		}
	}
}

void DynamicNormalizer::amplifyCurrentFrame(void)
{
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		if(p->maxAmplification[channel]->empty())
		{
			LOG_WRN(TXT("Second pass has more frames than the firts one -> passing trough unmodified!"));
			break;
		}

		const double currentAmplificationFactor = p->maxAmplification[channel]->front();
		p->maxAmplification[channel]->pop_front();

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			p->frameBuffer[channel][i] *= currentAmplificationFactor;
			if(p->frameBuffer[channel][i] > m_peakValue)
			{
				p->frameBuffer[channel][i] = m_peakValue; /*fix clipping*/
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

double DynamicNormalizer::findPeakMagnitude(const uint32_t channel)
{
	double dMax = -1.0;

	if(channel == UINT32_MAX)
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				dMax = std::max(dMax, abs(p->frameBuffer[c][i]));
			}
		}
	}
	else
	{
		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			dMax = std::max(dMax, abs(p->frameBuffer[channel][i]));
		}
	}

	return dMax;
}

void DynamicNormalizer::writeToLogFile(void)
{
	assert(m_logFile != NULL);

	for(size_t i = 0; i < p->maxAmplification[0]->size(); i++)
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			fprintf(m_logFile, channel ? "\t%.4f" : "%.4f", p->maxAmplification[channel]->at(i));
		}
		fprintf(m_logFile, "\n");
	}

	fprintf(m_logFile, "\n------\n\n");
}

/*
void DynamicNormalizer::perfromDCCorrection(void)
{
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		const double diff = 1.0 / double(m_frameLen);
		double currentAverageValue = 0.0;

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			currentAverageValue += (m_frameBuffer[channel][i] * diff);
		}

		m_channelAverageValue[channel] = UPDATE_VALUE(currentAverageValue, m_channelAverageValue[channel], m_aggressiveness);

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			m_frameBuffer[channel][i] -= m_channelAverageValue[channel];
		}
	}
}
*/
