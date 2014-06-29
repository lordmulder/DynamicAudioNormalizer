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

#include "DynamicAudioNormalizer.h"

#include "Common.h"
#include "RingBuffer.h"
#include "MinimumFilter.h"
#include "GaussianFilter.h"
#include "Version.h"

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
// DynamicAudioNormalizer_PrivateData
///////////////////////////////////////////////////////////////////////////////

class DynamicAudioNormalizer_PrivateData
{
public:
	DynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose, FILE *const logFile);
	~DynamicAudioNormalizer_PrivateData(void);

	bool processInplace(double **samplesIn, int64_t inputSize, int64_t &outputSize);
	bool setPass(const int pass);

private:
	const uint32_t m_channels;
	const uint32_t m_sampleRate;
	const uint32_t m_frameLen;
	const uint32_t m_filterSize;

	const bool m_channelsCoupled;
	const bool m_enableDCCorrection;

	const double m_peakValue;
	const double m_maxAmplification;

	const bool m_verbose;
	FILE *const m_logFile;

	int m_currentPass;
	std::deque<double> **m_frameHistory;
	double **m_frameBuffer;

	RingBuffer **m_bufferSrc;
	RingBuffer **m_bufferOut;

	MinimumFilter *m_minimumFilter;
	GaussianFilter *m_gaussFilter;

protected:
	void processNextFrame(void);
	void analyzeCurrentFrame(void);
	void amplifyCurrentFrame(void);
	
	void initializeSecondPass(void);
	double findPeakMagnitude(const uint32_t channel = UINT32_MAX);
	void perfromDCCorrection(void);
	void writeToLogFile(const char* info);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

DynamicAudioNormalizer::DynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose, FILE *const logFile)
:
	p(new DynamicAudioNormalizer_PrivateData(channels, sampleRate, frameLenMsec, channelsCoupled, enableDCCorrection, peakValue, maxAmplification, filterSize, verbose, logFile))

{
	/*nothing to do here*/
}

DynamicAudioNormalizer_PrivateData::DynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose, FILE *const logFile)
:
	m_channels(channels),
	m_sampleRate(sampleRate),
	m_frameLen(static_cast<uint32_t>(round(double(sampleRate) * (double(frameLenMsec) / 1000.0)))),
	m_channelsCoupled(channelsCoupled),
	m_enableDCCorrection(enableDCCorrection),
	m_peakValue(peakValue),
	m_maxAmplification(maxAmplification),
	m_filterSize(filterSize),
	m_verbose(verbose),
	m_logFile(logFile)
{
	m_currentPass = -1;

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

	m_bufferSrc = new RingBuffer*[channels];
	m_bufferOut = new RingBuffer*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_bufferSrc[channel] = new RingBuffer(m_frameLen);
		m_bufferOut[channel] = new RingBuffer(m_frameLen);
	}

	m_frameBuffer = new double*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_frameBuffer[channel] = new double[m_frameLen];
	}

	m_frameHistory = new std::deque<double>*[channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_frameHistory[channel] = new std::deque<double>();
	}

	const double sigma = (((double(m_filterSize) / 2.0) - 1.0) / 3.0) + (1.0 / 3.0);
	m_gaussFilter = new GaussianFilter(m_filterSize, sigma);

	const uint32_t minFilterSize = (m_filterSize * 2) / 3;
	m_minimumFilter = new MinimumFilter(minFilterSize + ((minFilterSize + 1) % 2));

	if(m_verbose)
	{
		LOG_DBG(TXT("[Parameters]"));
		LOG_DBG(TXT("Frame size     : %u"),     m_frameLen);
		LOG_DBG(TXT("Channels       : %u"),     m_channels);
		LOG_DBG(TXT("Sampling rate  : %u"),     m_sampleRate);
		LOG_DBG(TXT("Chan. coupling : %s"),     m_channelsCoupled    ? TXT("YES") : TXT("NO"));
		LOG_DBG(TXT("DC correction  : %s"),     m_enableDCCorrection ? TXT("YES") : TXT("NO"));
		LOG_DBG(TXT("Peak value     : %.4f"),   m_peakValue);
		LOG_DBG(TXT("Max amp factor : %.4f\n"), m_maxAmplification);
		LOG_DBG(TXT(""));
	}
}

DynamicAudioNormalizer::~DynamicAudioNormalizer(void)
{
	delete p;
}

DynamicAudioNormalizer_PrivateData::~DynamicAudioNormalizer_PrivateData(void)
{
	MY_DELETE(m_gaussFilter);
	MY_DELETE(m_minimumFilter);

	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		MY_DELETE_ARRAY(m_bufferSrc[channel]);
		MY_DELETE_ARRAY(m_bufferOut[channel]);
		MY_DELETE_ARRAY(m_frameBuffer[channel]);
		MY_DELETE(m_frameHistory[channel]);
	}

	MY_DELETE_ARRAY(m_bufferSrc);
	MY_DELETE_ARRAY(m_bufferOut);
	MY_DELETE_ARRAY(m_frameBuffer);
	MY_DELETE_ARRAY(m_frameHistory);
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool DynamicAudioNormalizer::processInplace(double **samples, int64_t inputSize, int64_t &outputSize)
{
	return p->processInplace(samples, inputSize, outputSize);
}

bool DynamicAudioNormalizer_PrivateData::processInplace(double **samples, int64_t inputSize, int64_t &outputSize)
{
	if((m_currentPass != DynamicAudioNormalizer::PASS_1ST) && (m_currentPass != DynamicAudioNormalizer::PASS_2ND))
	{
		LOG_ERR(TXT("Processing pass has not been set yet!"));
		return false;
	}

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

	if(inputSamplesLeft > 0)
	{
		LOG_WRN(TXT("No all input samples could be processed -> discarding pending input!"));
		return false;
	}

	return true;
}

bool DynamicAudioNormalizer::setPass(const int pass)
{
	return p->setPass(pass);
}

bool DynamicAudioNormalizer_PrivateData::setPass(const int pass)
{
	if((pass != DynamicAudioNormalizer::PASS_1ST) && (pass != DynamicAudioNormalizer::PASS_2ND))
	{
		LOG_ERR(TXT("Invalid pass value %d specified -> ignoring!"), pass);
		return false;
	}

	//Clear ring buffers
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_bufferSrc[channel]->clear();
		m_bufferOut[channel]->clear();
	}

	//Setup the new processing pass
	if(pass == DynamicAudioNormalizer::PASS_1ST)
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			m_frameHistory[channel]->clear();
		}
	}
	else
	{
		if(m_frameHistory[0]->size() < 1)
		{
			LOG_WRN(TXT("No information from 1st pass stored yet!"));
		}
		else
		{
			initializeSecondPass();
		}
	}

	m_currentPass = pass;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizer::getVersionInfo(uint32_t &major, uint32_t &minor,uint32_t &patch)
{
	major = DYAUNO_VERSION_MAJOR;
	minor = DYAUNO_VERSION_MINOR;
	minor = DYAUNO_VERSION_PATCH;
}

void DynamicAudioNormalizer::getBuildInfo(const char **date, const char **time, const char **compiler, const char **arch)
{
	*date     = DYAUNO_BUILD_DATE;
	*time     = DYAUNO_BUILD_TIME;
	*compiler = DYAUNO_COMPILER;
	*arch     = DYAUNO_ARCH;
}

///////////////////////////////////////////////////////////////////////////////
// Procesing Functions
///////////////////////////////////////////////////////////////////////////////

void DynamicAudioNormalizer_PrivateData::processNextFrame(void)
{
	//Run DC correction
	if(m_enableDCCorrection)
	{
		perfromDCCorrection();
	}
	
	switch(m_currentPass)
	{
	case DynamicAudioNormalizer::PASS_1ST:
		analyzeCurrentFrame();
		break;
	case DynamicAudioNormalizer::PASS_2ND:
		amplifyCurrentFrame();
		break;
	default:
		MY_THROW("Invalid pass value detected!");
	}
}

void DynamicAudioNormalizer_PrivateData::analyzeCurrentFrame(void)
{
	if(m_channelsCoupled)
	{
		const double peakMagnitude = findPeakMagnitude();
		const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
		
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			m_frameHistory[channel]->push_back(currentAmplificationFactor);
		}
	}
	else
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			const double peakMagnitude = findPeakMagnitude(channel);
			const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
			m_frameHistory[channel]->push_back(currentAmplificationFactor);
		}
	}
}

void DynamicAudioNormalizer_PrivateData::amplifyCurrentFrame(void)
{
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		if(m_frameHistory[channel]->empty())
		{
			LOG_WRN(TXT("Second pass has more frames than the firts one -> passing trough unmodified!"));
			break;
		}

		const double currentAmplificationFactor = m_frameHistory[channel]->front();
		m_frameHistory[channel]->pop_front();

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			m_frameBuffer[channel][i] *= currentAmplificationFactor;
			if(abs(m_frameBuffer[channel][i]) > m_peakValue)
			{
				m_frameBuffer[channel][i] = std::copysign(m_peakValue, m_frameBuffer[channel][i]); /*fix clipping*/
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

double DynamicAudioNormalizer_PrivateData::findPeakMagnitude(const uint32_t channel)
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

void DynamicAudioNormalizer_PrivateData::initializeSecondPass(void)
{
	writeToLogFile("ORIGINAL VALUES:");
	
	//Apply minimum filter
	for(uint32_t c = 0; c < m_channels; c++)
	{
		m_minimumFilter->apply(m_frameHistory[c]);
	}

	writeToLogFile("MINIMUM FILTERED:");

	//Apply Gaussian smooth filter
	for(uint32_t c = 0; c < m_channels; c++)
	{
		m_gaussFilter->apply(m_frameHistory[c], 1.0);
	}

	writeToLogFile("GAUSS FILTERED:");
}

void DynamicAudioNormalizer_PrivateData::writeToLogFile(const char* info)
{
	if(!m_logFile)
	{
		return; /*no logile specified*/
	}

	fprintf(m_logFile, "\n%s\n\n", info);

	for(size_t i = 0; i < m_frameHistory[0]->size(); i++)
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			fprintf(m_logFile, channel ? "\t%.4f" : "%.4f", m_frameHistory[channel]->at(i));
		}
		fprintf(m_logFile, "\n");
	}

	fprintf(m_logFile, "\n------\n\n");
}

void DynamicAudioNormalizer_PrivateData::perfromDCCorrection(void)
{
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		const double diff = 1.0 / double(m_frameLen);
		double currentAverageValue = 0.0;

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			currentAverageValue += (m_frameBuffer[channel][i] * diff);
		}

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			m_frameBuffer[channel][i] -= currentAverageValue;
		}
	}
}
