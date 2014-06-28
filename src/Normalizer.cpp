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

#include "Normalizer.h"

#include <cmath>
#include <algorithm>
#include <cassert>

static const double MAX_AMPLIFICATION = 10.0;

static inline double UPDATE_VALUE(const double &NEW, const double &OLD, const double &aggressiveness)
{
	assert((aggressiveness >= 0.0) && (aggressiveness <= 1.0));
	return (aggressiveness * NEW) + ((1.0 - aggressiveness) * OLD);
}

Normalizer::Normalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double aggressiveness, FILE *const logFile)
:
	m_channels(channels),
	m_sampleRate(sampleRate),
	m_frameLen(static_cast<uint32_t>(round(double(sampleRate) * (double(frameLenMsec) / 1000.0)))),
	m_channelsCoupled(channelsCoupled),
	m_enableDCCorrection(enableDCCorrection),
	m_peakValue(peakValue),
	m_aggressiveness(aggressiveness),
	m_logFile(logFile)
{
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

	if((m_aggressiveness < 0.0) || (m_aggressiveness > 1.0))
	{
		MY_THROW("Invalid aggressiveness value detected. Should be in the 0.0 to 1.0 range!");
	}

	m_bufferSrc = new RingBuffer*[channels];
	m_bufferOut = new RingBuffer*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_bufferSrc[channel] = new RingBuffer(2* m_frameLen);
		m_bufferOut[channel] = new RingBuffer(2* m_frameLen);
	}

	m_frameBuffer = new double*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_frameBuffer[channel] = new double[m_frameLen];
	}

	m_amplificationFactor = new double[channels];
	m_channelAverageValue = new double[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_amplificationFactor[channel] = 1.0;
		m_channelAverageValue[channel] = 0.0;
	}

	LOG_DBG(TXT("---------Parameters---------"));
	LOG_DBG(TXT("Frame size     : %u"),   m_frameLen);
	LOG_DBG(TXT("Channels       : %u"),   m_channels);
	LOG_DBG(TXT("Sampling rate  : %u"),   m_sampleRate);
	LOG_DBG(TXT("Chan. coupling : %s"),   m_channelsCoupled    ? TXT("YES") : TXT("NO"));
	LOG_DBG(TXT("DC correction  : %s"),   m_enableDCCorrection ? TXT("YES") : TXT("NO"));
	LOG_DBG(TXT("Peak Value     : %.4f"), m_peakValue);
	LOG_DBG(TXT("Aggressiveness : %.4f"), m_aggressiveness);
	LOG_DBG(TXT("---------Parameters---------\n"));
}

Normalizer::~Normalizer(void)
{
	MY_DELETE_ARRAY(m_amplificationFactor);
	MY_DELETE_ARRAY(m_channelAverageValue);

	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		MY_DELETE_ARRAY(m_frameBuffer[channel]);
	}
	MY_DELETE_ARRAY(m_frameBuffer);

	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		MY_DELETE_ARRAY(m_bufferSrc[channel]);
	}
	MY_DELETE_ARRAY(m_bufferSrc);

	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		MY_DELETE_ARRAY(m_bufferOut[channel]);
	}
	MY_DELETE_ARRAY(m_bufferOut);
}

void Normalizer::processInplace(double **samples, int64_t inputSize, int64_t &outputSize)
{
	outputSize = 0;
	bool bStop = false;

	uint32_t inputPos = 0;
	uint32_t inputSamplesLeft = static_cast<uint32_t>(std::min(std::max(inputSize, 0i64), int64_t(UINT32_MAX)));
	
	uint32_t outputPos = 0;
	uint32_t outputBufferLeft = 0;

	while(!bStop)
	{
		bStop = true;

		//Read input samples
		while((inputSamplesLeft > 0) && (m_bufferSrc[0]->getFree() > 0))
		{
			const uint32_t copyLen = std::min(inputSamplesLeft, m_bufferSrc[0]->getFree());
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				m_bufferSrc[channel]->append(&samples[channel][inputPos], copyLen);
			}
			inputPos += copyLen;
			inputSamplesLeft -= copyLen;
			outputBufferLeft += copyLen;
			bStop = false;
		}

		//Process frames
		while((m_bufferSrc[0]->getUsed() >= m_frameLen) && (m_bufferOut[0]->getFree() >= m_frameLen))
		{
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				m_bufferSrc[channel]->read(m_frameBuffer[channel], m_frameLen);
			}
			processNextFrame();
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				m_bufferOut[channel]->append(m_frameBuffer[channel], m_frameLen);
			}
			bStop = false;
		}

		//Write output samples
		while((outputBufferLeft > 0) && (m_bufferOut[0]->getUsed() > 0))
		{
			const uint32_t copyLen = std::min(outputBufferLeft, m_bufferOut[0]->getUsed());
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				m_bufferOut[channel]->read(&samples[channel][outputPos], copyLen);
			}
			outputPos += copyLen;
			outputBufferLeft -= copyLen;
			bStop = false;
		}
	}

	outputSize = int64_t(outputPos);
}

void Normalizer::processNextFrame(void)
{
	//DC correction
	if(m_enableDCCorrection)
	{
		perfromDCCorrection();
	}
	
	//Update factors
	updateAmplificationFactors();

	//Amplify samples
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			m_frameBuffer[channel][i] *= m_amplificationFactor[channel];
		}
	}

	//Append to the logfile
	if(m_logFile)
	{
		writeToLogFile();
	}
}

double Normalizer::findPeakMagnitude(const uint32_t channel)
{
	double dMax = -1.0;

	if(channel == UINT32_MAX)
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				dMax = std::max(dMax, abs(m_frameBuffer[c][i]));
			}
		}
	}
	else
	{
		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			dMax = std::max(dMax, abs(m_frameBuffer[channel][i]));
		}
	}

	return dMax;
}

void Normalizer::updateAmplificationFactors(void)
{
	if(m_channelsCoupled)
	{
		const double peakMagnitude = findPeakMagnitude();
		const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, MAX_AMPLIFICATION);

		if(currentAmplificationFactor > m_amplificationFactor[0])
		{
			m_amplificationFactor[0] = UPDATE_VALUE(currentAmplificationFactor, m_amplificationFactor[0], m_aggressiveness);
			for(uint32_t channel = 1; channel < m_channels; channel++)
			{
				m_amplificationFactor[channel] = m_amplificationFactor[channel - 1];
			}
		}
		else
		{
			/*clipping protection*/
			for(uint32_t channel = 0; channel < m_channels; channel++)
			{
				m_amplificationFactor[channel] = currentAmplificationFactor;
			}
		}
	}
	else
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			const double peakMagnitude = findPeakMagnitude(channel);
			const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, MAX_AMPLIFICATION);

			if(currentAmplificationFactor > m_amplificationFactor[0])
			{
				m_amplificationFactor[channel] = UPDATE_VALUE(currentAmplificationFactor, m_amplificationFactor[channel], m_aggressiveness);
			}
			else
			{
				/*clipping protection*/
				m_amplificationFactor[channel] = currentAmplificationFactor;
			}
		}
	}
}

void Normalizer::perfromDCCorrection(void)
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

void Normalizer::writeToLogFile(void)
{
	assert(m_logFile != NULL);

	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		fprintf(m_logFile, channel ? "\t%.4f" : "%.4f", m_amplificationFactor[channel]);
	}

	fprintf(m_logFile, "\t|\t");

	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		fprintf(m_logFile, channel ? "\t%.4f" : "%.4f", m_channelAverageValue[channel]);
	}

	fprintf(m_logFile, "\n");
}
