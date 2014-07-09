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
#include <deque>
#include <cassert>
#include <stdexcept> 

///////////////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////////////

#undef LOG_AMPFACTORS
#ifdef LOG_AMPFACTORS
static void LOG_VALUE(const double &value, const double &prev, const double &curr, const double &next, const uint32_t &channel, const uint32_t &pos)
{
	static FILE *g_myLogfile = FOPEN(TXT("ampFactors.log"), TXT("w"));
	if(g_myLogfile && (channel == 0) && (pos % 10 == 0))
	{
		fprintf(g_myLogfile, "%.8f\t%.8f %.8f %.8f\n", value, prev, curr, next);
	}
}
#else
#define LOG_VALUE(...) ((void)0)
#endif

static inline uint32_t FRAME_SIZE(const uint32_t &sampleRate, const uint32_t &frameLenMsec)
{
	const uint32_t frameSize = static_cast<uint32_t>(round(double(sampleRate) * (double(frameLenMsec) / 1000.0)));
	return frameSize + (frameSize % 2);
}

static inline double UPDATE_VALUE(const double &NEW, const double &OLD, const double &aggressiveness)
{
	assert((aggressiveness >= 0.0) && (aggressiveness <= 1.0));
	return (aggressiveness * NEW) + ((1.0 - aggressiveness) * OLD);
}

static inline double FADE(const double &prev, const double &curr, const double &next, const uint32_t &pos, const double *const fadeFactors[3])
{
	return (fadeFactors[0][pos] * prev) + (fadeFactors[1][pos] * curr) + (fadeFactors[2][pos] * next);
}

///////////////////////////////////////////////////////////////////////////////
// MDynamicAudioNormalizer_PrivateData
///////////////////////////////////////////////////////////////////////////////

class MDynamicAudioNormalizer_PrivateData
{
public:
	MDynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose, FILE *const logFile);
	~MDynamicAudioNormalizer_PrivateData(void);

	bool initialize(void);
	bool processInplace(double **samplesInOut, int64_t inputSize, int64_t &outputSize);
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
	
	bool m_initialized;
	bool m_firstFrameFlag;

	std::deque<double> **m_framePeakHistory;
	double **m_frameBuffer;
	double *m_prevAmplificationFactor;
	double *m_dcCorrectionValue;

	RingBuffer **m_bufferSrc;
	RingBuffer **m_bufferOut;

	MinimumFilter *m_minimumFilter;
	GaussianFilter *m_gaussFilter;

	double *m_fadeFactors[3];

protected:
	void processNextFrame(void);
	void analyzeCurrentFrame(void);
	void amplifyCurrentFrame(void);
	
	void initializeSecondPass(void);
	double findPeakMagnitude(const uint32_t channel = UINT32_MAX);
	void perfromDCCorrection(const bool &isFirstFrame);
	void precalculateFadeFactors(double *fadeFactors[3]);
	void writeToLogFile(const char* info);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

MDynamicAudioNormalizer::MDynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose, FILE *const logFile)
:
	p(new MDynamicAudioNormalizer_PrivateData(channels, sampleRate, frameLenMsec, channelsCoupled, enableDCCorrection, peakValue, maxAmplification, filterSize, verbose, logFile))

{
	/*nothing to do here*/
}

MDynamicAudioNormalizer_PrivateData::MDynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, const bool verbose, FILE *const logFile)
:
	m_channels(channels),
	m_sampleRate(sampleRate),
	m_frameLen(FRAME_SIZE(sampleRate, frameLenMsec)),
	m_filterSize(filterSize),
	m_channelsCoupled(channelsCoupled),
	m_enableDCCorrection(enableDCCorrection),
	m_peakValue(peakValue),
	m_maxAmplification(maxAmplification),
	m_verbose(verbose),
	m_logFile(logFile)
{
	m_initialized = false;
	m_firstFrameFlag = true;
	m_currentPass = -1;
	m_framePeakHistory = NULL;
	m_frameBuffer = NULL;
	m_prevAmplificationFactor = NULL;
	m_dcCorrectionValue = NULL;
	m_bufferSrc = NULL;
	m_bufferOut = NULL;
	m_minimumFilter = NULL;
	m_gaussFilter = NULL;
	m_fadeFactors[0] = NULL;
	m_fadeFactors[1] = NULL;
	m_fadeFactors[2] = NULL;
}

MDynamicAudioNormalizer::~MDynamicAudioNormalizer(void)
{
	delete p;
}

MDynamicAudioNormalizer_PrivateData::~MDynamicAudioNormalizer_PrivateData(void)
{
	MY_DELETE(m_gaussFilter);
	MY_DELETE(m_minimumFilter);

	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		if(m_bufferSrc)        MY_DELETE(m_bufferSrc[channel]);
		if(m_bufferOut)        MY_DELETE(m_bufferOut[channel]);
		if(m_frameBuffer)      MY_DELETE_ARRAY(m_frameBuffer[channel]);
		if(m_framePeakHistory) MY_DELETE(m_framePeakHistory[channel]);
	}

	MY_DELETE_ARRAY(m_bufferSrc);
	MY_DELETE_ARRAY(m_bufferOut);
	MY_DELETE_ARRAY(m_prevAmplificationFactor);
	MY_DELETE_ARRAY(m_dcCorrectionValue);
	MY_DELETE_ARRAY(m_frameBuffer);
	MY_DELETE_ARRAY(m_framePeakHistory);
	MY_DELETE_ARRAY(m_fadeFactors[0]);
	MY_DELETE_ARRAY(m_fadeFactors[1]);
	MY_DELETE_ARRAY(m_fadeFactors[2]);
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool MDynamicAudioNormalizer::initialize(void)
{
	try
	{
		return p->initialize();
	}
	catch(std::exception &e)
	{
		LOG_ERR(FMT_CHAR, e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::initialize(void)
{
	if(m_initialized)
	{
		LOG_WRN(TXT("%s"), TXT("Already initialized -> ignoring!"));
		return true;
	}

	if((m_channels < 1) || (m_channels > 8))
	{
		LOG_ERR(TXT("Invalid or unsupported channel count. Should be in the %d to %d range!"), 1, 8);
		return false;
	}
	if((m_sampleRate < 11025) || (m_channels > 192000))
	{
		LOG_ERR(TXT("Invalid or unsupported sampling rate. Should be in the %d to %d range!"), 11025, 192000);
		return false;
	}
	if((m_frameLen < 32) || (m_frameLen > 2097152))
	{
		LOG_ERR(TXT("Invalid or unsupported frame size. Should be in the %d to %d range!"), 32, 2097152);
		return false;
	}

	m_bufferSrc = new RingBuffer*[m_channels];
	m_bufferOut = new RingBuffer*[m_channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_bufferSrc[channel] = new RingBuffer(m_frameLen);
		m_bufferOut[channel] = new RingBuffer(m_frameLen);
	}

	m_frameBuffer = new double*[m_channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_frameBuffer[channel] = new double[m_frameLen];
	}

	m_framePeakHistory = new std::deque<double>*[m_channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_framePeakHistory[channel] = new std::deque<double>();
	}

	m_prevAmplificationFactor = new double[m_channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_prevAmplificationFactor[channel] = 1.0;
	}

	m_dcCorrectionValue = new double[m_channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_dcCorrectionValue[channel] = 0.0;
	}

	m_fadeFactors[0] = new double[m_frameLen];
	m_fadeFactors[1] = new double[m_frameLen];
	m_fadeFactors[2] = new double[m_frameLen];
	
	precalculateFadeFactors(m_fadeFactors);

	const double sigma = (((double(m_filterSize) / 2.0) - 1.0) / 3.0) + (1.0 / 3.0);
	m_minimumFilter = new MinimumFilter(m_filterSize);
	m_gaussFilter = new GaussianFilter(m_filterSize, sigma);

	if(m_verbose)
	{
		LOG_DBG(TXT("%s"), TXT("[PARAMETERS]"));
		LOG_DBG(TXT("Frame size     : %u"),     m_frameLen);
		LOG_DBG(TXT("Channels       : %u"),     m_channels);
		LOG_DBG(TXT("Sampling rate  : %u"),     m_sampleRate);
		LOG_DBG(TXT("Chan. coupling : %s"),     m_channelsCoupled    ? TXT("YES") : TXT("NO"));
		LOG_DBG(TXT("DC correction  : %s"),     m_enableDCCorrection ? TXT("YES") : TXT("NO"));
		LOG_DBG(TXT("Peak value     : %.4f"),   m_peakValue);
		LOG_DBG(TXT("Max amp factor : %.4f\n"), m_maxAmplification);
	}

	m_initialized = true;
	return true;
}

bool MDynamicAudioNormalizer::processInplace(double **samplesInOut, int64_t inputSize, int64_t &outputSize)
{
	try
	{
		return p->processInplace(samplesInOut, inputSize, outputSize);
	}
	catch(std::exception &e)
	{
		LOG_ERR(FMT_CHAR, e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::processInplace(double **samplesInOut, int64_t inputSize, int64_t &outputSize)
{
	outputSize = 0;

	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG_ERR(TXT("%s"), TXT("Not initialized yet. Must call initialize() first!"));
		return false;
	}
	if((m_currentPass != MDynamicAudioNormalizer::PASS_1ST) && (m_currentPass != MDynamicAudioNormalizer::PASS_2ND))
	{
		LOG_ERR(TXT("%s"), TXT("Pass has not been set yet. Must call setPass() first!"));
		return false;
	}

	uint32_t inputPos = 0;
	uint32_t inputSamplesLeft = static_cast<uint32_t>(std::min(std::max(inputSize, int64_t(0)), int64_t(UINT32_MAX)));
	
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
				m_bufferSrc[channel]->append(&samplesInOut[channel][inputPos], copyLen);
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
				m_bufferOut[channel]->read(&samplesInOut[channel][outputPos], copyLen);
			}
			outputPos += copyLen;
			outputBufferLeft -= copyLen;
			bStop = false;
		}
	}

	outputSize = int64_t(outputPos);

	if(inputSamplesLeft > 0)
	{
		LOG_WRN(TXT("%s"), TXT("No all input samples could be processed -> discarding pending input!"));
		return false;
	}

	return true;
}

bool MDynamicAudioNormalizer::setPass(const int pass)
{
	try
	{
		return p->setPass(pass);
	}
	catch(std::exception &e)
	{
		LOG_ERR(FMT_CHAR, e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::setPass(const int pass)
{
	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG_ERR(TXT("%s"), TXT("Not initialized yet. Must call initialize() first!"));
		return false;
	}
	if((pass != MDynamicAudioNormalizer::PASS_1ST) && (pass != MDynamicAudioNormalizer::PASS_2ND))
	{
		LOG_ERR(TXT("Invalid pass value %d specified -> ignoring!"), pass);
		return false;
	}

	//Reset flag
	m_firstFrameFlag = true;

	//Clear ring buffers
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_bufferSrc[channel]->clear();
		m_bufferOut[channel]->clear();
	}

	//Reset DC correction
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_dcCorrectionValue[channel] = 0.0;
	}

	//Setup the new processing pass
	if(pass == MDynamicAudioNormalizer::PASS_1ST)
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			m_framePeakHistory[channel]->clear();
		}
	}
	else
	{
		if(m_framePeakHistory[0]->size() < 1)
		{
			LOG_WRN(TXT("%s"), TXT("No information from 1st pass stored yet!"));
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

void MDynamicAudioNormalizer::getVersionInfo(uint32_t &major, uint32_t &minor,uint32_t &patch)
{
	major = DYAUNO_VERSION_MAJOR;
	minor = DYAUNO_VERSION_MINOR;
	patch = DYAUNO_VERSION_PATCH;
}

void MDynamicAudioNormalizer::getBuildInfo(const char **date, const char **time, const char **compiler, const char **arch, bool &debug)
{
	*date     = DYAUNO_BUILD_DATE;
	*time     = DYAUNO_BUILD_TIME;
	*compiler = DYAUNO_COMPILER;
	*arch     = DYAUNO_ARCH;
	debug     = bool(DYAUNO_DEBUG);
}

///////////////////////////////////////////////////////////////////////////////
// Procesing Functions
///////////////////////////////////////////////////////////////////////////////

void MDynamicAudioNormalizer_PrivateData::processNextFrame(void)
{
	//Run DC correction
	if(m_enableDCCorrection)
	{
		perfromDCCorrection(m_firstFrameFlag);
	}
	
	//Perfrom action for current pass
	switch(m_currentPass)
	{
	case MDynamicAudioNormalizer::PASS_1ST:
		analyzeCurrentFrame();
		break;
	case MDynamicAudioNormalizer::PASS_2ND:
		amplifyCurrentFrame();
		break;
	default:
		throw std::runtime_error("Invalid pass value detected!");
	}

	//Clear flag
	m_firstFrameFlag = false;
}

void MDynamicAudioNormalizer_PrivateData::analyzeCurrentFrame(void)
{
	if(m_channelsCoupled)
	{
		const double peakMagnitude = findPeakMagnitude();
		const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
		
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			m_framePeakHistory[channel]->push_back(currentAmplificationFactor);
		}
	}
	else
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			const double peakMagnitude = findPeakMagnitude(channel);
			const double currentAmplificationFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
			m_framePeakHistory[channel]->push_back(currentAmplificationFactor);
		}
	}
}

void MDynamicAudioNormalizer_PrivateData::amplifyCurrentFrame(void)
{
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		if(m_framePeakHistory[channel]->empty())
		{
			LOG_WRN(TXT("%s"), TXT("Second pass has more frames than the firts one -> passing trough unmodified!"));
			break;
		}

		const double currAmplificationFactor = m_framePeakHistory[channel]->front();
		m_framePeakHistory[channel]->pop_front();
		const double nextAmplificationFactor = m_framePeakHistory[channel]->empty() ? currAmplificationFactor : m_framePeakHistory[channel]->front();

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			const double amplificationFactor = FADE(m_prevAmplificationFactor[channel], currAmplificationFactor, nextAmplificationFactor, i, m_fadeFactors);
			m_frameBuffer[channel][i] *= amplificationFactor;
			LOG_VALUE(amplificationFactor, m_prevAmplificationFactor[channel], currAmplificationFactor, nextAmplificationFactor, channel, i);
			if(fabs(m_frameBuffer[channel][i]) > m_peakValue)
			{
				m_frameBuffer[channel][i] = copysign(m_peakValue, m_frameBuffer[channel][i]); /*fix rare clipping*/
			}
		}

		m_prevAmplificationFactor[channel] = currAmplificationFactor;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

double MDynamicAudioNormalizer_PrivateData::findPeakMagnitude(const uint32_t channel)
{
	double dMax = -1.0;

	if(channel == UINT32_MAX)
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				dMax = std::max(dMax, fabs(m_frameBuffer[c][i]));
			}
		}
	}
	else
	{
		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			dMax = std::max(dMax, fabs(m_frameBuffer[channel][i]));
		}
	}

	return dMax;
}

void MDynamicAudioNormalizer_PrivateData::initializeSecondPass(void)
{
	writeToLogFile("ORIGINAL VALUES:");
	
	//Apply minimum filter
	for(uint32_t c = 0; c < m_channels; c++)
	{
		m_minimumFilter->apply(m_framePeakHistory[c]);
	}

	writeToLogFile("MINIMUM FILTERED:");

	//Apply Gaussian smooth filter
	for(uint32_t c = 0; c < m_channels; c++)
	{
		m_gaussFilter->apply(m_framePeakHistory[c], 1.0);
	}

	//Set initial factors
	for(uint32_t c = 0; c < m_channels; c++)
	{
		m_prevAmplificationFactor[c] = m_framePeakHistory[c]->front();
	}

	writeToLogFile("GAUSS FILTERED:");
}

void MDynamicAudioNormalizer_PrivateData::writeToLogFile(const char* info)
{
	if(!m_logFile)
	{
		return; /*no logile specified*/
	}

	fprintf(m_logFile, "\n%s\n\n", info);

	for(size_t i = 0; i < m_framePeakHistory[0]->size(); i++)
	{
		for(uint32_t channel = 0; channel < m_channels; channel++)
		{
			fprintf(m_logFile, channel ? "\t%.4f" : "%.4f", m_framePeakHistory[channel]->at(i));
		}
		fprintf(m_logFile, "\n");
	}

	fprintf(m_logFile, "\n------\n\n");
}

void MDynamicAudioNormalizer_PrivateData::perfromDCCorrection(const bool &isFirstFrame)
{
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		const double diff = 1.0 / double(m_frameLen);
		double currentAverageValue = 0.0;

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			currentAverageValue += (m_frameBuffer[channel][i] * diff);
		}

		const double prevCorrectionValue = m_dcCorrectionValue[channel];
		m_dcCorrectionValue[channel] = isFirstFrame ? currentAverageValue : UPDATE_VALUE(currentAverageValue, m_dcCorrectionValue[channel], 0.1);

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			m_frameBuffer[channel][i] -= FADE(prevCorrectionValue, m_dcCorrectionValue[channel], m_dcCorrectionValue[channel], i, m_fadeFactors);
		}
	}
}

void MDynamicAudioNormalizer_PrivateData::precalculateFadeFactors(double *fadeFactors[3])
{
	assert((m_frameLen % 2) == 0);

	const uint32_t frameFadeDiv2 = m_frameLen / 2U;
	const double frameFadeStep = 0.5 / double(frameFadeDiv2);

	for(uint32_t pos = 0; pos < frameFadeDiv2; pos++)
	{
		fadeFactors[0][pos] = 0.5 - (frameFadeStep * double(pos));
		fadeFactors[2][pos] = 0.0;
		fadeFactors[1][pos] = 1.0 - fadeFactors[0][pos] - fadeFactors[2][pos];
	}

	for(uint32_t pos = frameFadeDiv2; pos < m_frameLen; pos++)
	{
		fadeFactors[2][pos] = frameFadeStep * double(pos % frameFadeDiv2);
		fadeFactors[0][pos] = 0.0;
		fadeFactors[1][pos] = 1.0 - fadeFactors[0][pos] - fadeFactors[2][pos];
	}

	//for(uint32_t pos = 0; pos < m_frameLen; pos++)
	//{
	//	LOG_DBG(TXT("%.8f %.8f %.8f"), fadeFactors[0][pos], fadeFactors[1][pos], fadeFactors[2][pos]);
	//}
}
