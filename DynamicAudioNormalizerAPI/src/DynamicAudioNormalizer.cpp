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
#include "FrameBuffer.h"
#include "MinimumFilter.h"
#include "GaussianFilter.h"
#include "Logging.h"
#include "Version.h"

#include <cmath>
#include <algorithm>
#include <deque>
#include <cassert>
#include <stdexcept> 
#include <cfloat>

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
	MDynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, FILE *const logFile);
	~MDynamicAudioNormalizer_PrivateData(void);

	bool initialize(void);
	bool processInplace(double **samplesInOut, const int64_t inputSize, int64_t &outputSize, const bool &bFlush);
	bool flushBuffer(double **samplesOut, const int64_t bufferSize, int64_t &outputSize);
	bool reset(void);

private:
	const uint32_t m_channels;
	const uint32_t m_sampleRate;
	const uint32_t m_frameLen;
	const uint32_t m_filterSize;

	const bool m_channelsCoupled;
	const bool m_enableDCCorrection;

	const double m_peakValue;
	const double m_maxAmplification;

	FILE *const m_logFile;
	
	bool m_initialized;
	bool m_flushBuffer;

	FrameFIFO *m_buffSrc;
	FrameFIFO *m_buffOut;
	
	int64_t m_delayedSamples;

	FrameBuffer *m_frameBuffer;
	
	std::deque<double> *m_gainHistory_original;
	std::deque<double> *m_gainHistory_minimum;
	std::deque<double> *m_gainHistory_smoothed;

	MinimumFilter  *m_minimumFilter;
	GaussianFilter *m_gaussianFilter;

	double *m_prevAmplificationFactor;
	double *m_dcCorrectionValue;

	double *m_fadeFactors[3];

protected:
	void processNextFrame(void);
	void analyzeFrame(FrameData *frame);
	void amplifyFrame(FrameData *frame);
	
	void precalculateFadeFactors(double *fadeFactors[3]);
	double findPeakMagnitude(FrameData *frame, const uint32_t channel = UINT32_MAX);
	void updateGainHistory(const uint32_t &channel, const double &currentGainFactor);
	void perfromDCCorrection(FrameData *frame, const bool &isFirstFrame);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

MDynamicAudioNormalizer::MDynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, FILE *const logFile)
:
	p(new MDynamicAudioNormalizer_PrivateData(channels, sampleRate, frameLenMsec, channelsCoupled, enableDCCorrection, peakValue, maxAmplification, filterSize, logFile))

{
	/*nothing to do here*/
}

MDynamicAudioNormalizer_PrivateData::MDynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const bool channelsCoupled, const bool enableDCCorrection, const double peakValue, const double maxAmplification, const uint32_t filterSize, FILE *const logFile)
:
	m_channels(channels),
	m_sampleRate(sampleRate),
	m_frameLen(FRAME_SIZE(sampleRate, frameLenMsec)),
	m_filterSize(filterSize),
	m_channelsCoupled(channelsCoupled),
	m_enableDCCorrection(enableDCCorrection),
	m_peakValue(peakValue),
	m_maxAmplification(maxAmplification),
	m_logFile(logFile)
{
	m_initialized = false;
	m_flushBuffer = false;
	
	m_buffSrc = NULL;
	m_buffOut = NULL;
	
	m_delayedSamples = 0;

	m_frameBuffer = NULL;

	m_gainHistory_original = NULL;
	m_gainHistory_minimum  = NULL;
	m_gainHistory_smoothed = NULL;

	m_minimumFilter  = NULL;
	m_gaussianFilter = NULL;

	m_prevAmplificationFactor = NULL;
	m_dcCorrectionValue = NULL;

	m_fadeFactors[0] = m_fadeFactors[1] = m_fadeFactors[2] = NULL;
}

MDynamicAudioNormalizer::~MDynamicAudioNormalizer(void)
{
	delete p;
}

MDynamicAudioNormalizer_PrivateData::~MDynamicAudioNormalizer_PrivateData(void)
{
	MY_DELETE(m_buffSrc);
	MY_DELETE(m_buffOut);

	MY_DELETE(m_frameBuffer);

	MY_DELETE(m_minimumFilter);
	MY_DELETE(m_gaussianFilter);
	
	MY_DELETE_ARRAY(m_gainHistory_original);
	MY_DELETE_ARRAY(m_gainHistory_minimum );
	MY_DELETE_ARRAY(m_gainHistory_smoothed);

	MY_DELETE_ARRAY(m_prevAmplificationFactor);
	MY_DELETE_ARRAY(m_dcCorrectionValue);

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
		LOG_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::initialize(void)
{
	if(m_initialized)
	{
		LOG_WRN("Already initialized -> ignoring!");
		return true;
	}

	if((m_channels < 1) || (m_channels > 8))
	{
		LOG2_ERR("Invalid or unsupported channel count. Should be in the %d to %d range!", 1, 8);
		return false;
	}
	if((m_sampleRate < 11025) || (m_channels > 192000))
	{
		LOG2_ERR("Invalid or unsupported sampling rate. Should be in the %d to %d range!", 11025, 192000);
		return false;
	}
	if((m_frameLen < 32) || (m_frameLen > 2097152))
	{
		LOG2_ERR("Invalid or unsupported frame size. Should be in the %d to %d range!", 32, 2097152);
		return false;
	}

	m_buffSrc = new FrameFIFO(m_channels, m_frameLen);
	m_buffOut = new FrameFIFO(m_channels, m_frameLen);

	m_frameBuffer = new FrameBuffer(m_channels, m_frameLen, m_filterSize + 1);

	m_gainHistory_original = new std::deque<double>[m_channels];
	m_gainHistory_minimum  = new std::deque<double>[m_channels];
	m_gainHistory_smoothed = new std::deque<double>[m_channels];

	m_minimumFilter = new MinimumFilter(m_filterSize);
	const double sigma = (((double(m_filterSize) / 2.0) - 1.0) / 3.0) + (1.0 / 3.0);
	m_gaussianFilter = new GaussianFilter(m_filterSize, sigma);

	m_dcCorrectionValue       = new double[m_channels];
	m_prevAmplificationFactor = new double[m_channels];
	for(uint32_t channel = 0; channel < m_channels; channel++)
	{
		m_dcCorrectionValue      [channel] = 0.0;
		m_prevAmplificationFactor[channel] = 1.0;
	}

	m_fadeFactors[0] = new double[m_frameLen];
	m_fadeFactors[1] = new double[m_frameLen];
	m_fadeFactors[2] = new double[m_frameLen];
	
	precalculateFadeFactors(m_fadeFactors);

	LOG_DBG ("[PARAMETERS]");
	LOG2_DBG("Frame size     : %u",     m_frameLen);
	LOG2_DBG("Channels       : %u",     m_channels);
	LOG2_DBG("Sampling rate  : %u",     m_sampleRate);
	LOG2_DBG("Chan. coupling : %s",     m_channelsCoupled    ? "YES" : "NO");
	LOG2_DBG("DC correction  : %s",     m_enableDCCorrection ? "YES" : "NO");
	LOG2_DBG("Peak value     : %.4f",   m_peakValue);
	LOG2_DBG("Max amp factor : %.4f\n", m_maxAmplification);

	m_initialized = true;
	reset();
	return true;
}

bool MDynamicAudioNormalizer::reset(void)
{
	try
	{
		return p->reset();
	}
	catch(std::exception &e)
	{
		LOG_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::reset(void)
{
	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}

	m_delayedSamples = 0;
	m_flushBuffer = false;

	m_buffSrc->reset();
	m_buffSrc->reset();

	m_frameBuffer->reset();

	m_gainHistory_original->clear();
	m_gainHistory_minimum ->clear();
	m_gainHistory_smoothed->clear();

	return true;
}

bool MDynamicAudioNormalizer::processInplace(double **samplesInOut, const int64_t inputSize, int64_t &outputSize)
{
	try
	{
		return p->processInplace(samplesInOut, inputSize, outputSize, false);
	}
	catch(std::exception &e)
	{
		LOG_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::processInplace(double **samplesInOut, const int64_t inputSize, int64_t &outputSize, const bool &bFlush)
{
	outputSize = 0;

	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}
	if(m_flushBuffer && (!bFlush))
	{
		LOG_ERR("Must not call processInplace() after flushBuffer(). Call reset() first!");
		return false;
	}

	bool bStop = false;

	uint32_t inputPos = 0;
	uint32_t inputSamplesLeft = static_cast<uint32_t>(std::min(std::max(inputSize, int64_t(0)), int64_t(UINT32_MAX)));
	
	uint32_t outputPos = 0;
	uint32_t outputBufferLeft = 0;

	while(!bStop)
	{
		bStop = true;

		//Read as many input samples as possible
		while((inputSamplesLeft > 0) && (m_buffSrc->samplesLeftPut() > 0))
		{
			bStop = false;
			
			const uint32_t copyLen = std::min(inputSamplesLeft, m_buffSrc->samplesLeftPut());
			m_buffSrc->putSamples(samplesInOut, inputPos, copyLen);

			inputPos         += copyLen;
			inputSamplesLeft -= copyLen;
			outputBufferLeft += copyLen;
		}

		//Analyze next input frame, if we have enough input
		if(m_buffSrc->samplesLeftGet() >= m_frameLen)
		{
			bStop = false;
			analyzeFrame(m_buffSrc->data());
			
			if(!m_frameBuffer->putFrame(m_buffSrc))
			{
				LOG_ERR("Failed to append current input frame to internal buffer!");
				return false;
			}

			m_buffSrc->reset();
		}

		//Amplify next output frame, if we have enough output
		if((m_buffOut->samplesLeftPut() >= m_frameLen) && (m_frameBuffer->framesUsed() > 0) && (!m_gainHistory_smoothed[0].empty()))
		{
			bStop = false;
			
			if(!m_frameBuffer->getFrame(m_buffOut))
			{
				LOG_ERR("Failed to retrieve next output frame from internal buffer!");
				return false;
			}

			amplifyFrame(m_buffOut->data());
		}

		//Write as many output samples as possible
		while((outputBufferLeft > 0) && (m_buffOut->samplesLeftGet() > 0))
		{
			bStop = false;

			const uint32_t copyLen = std::min(outputBufferLeft, m_buffOut->samplesLeftGet());
			m_buffOut->getSamples(samplesInOut, outputPos, copyLen);

			outputPos        += copyLen;
			outputBufferLeft -= copyLen;

			if((m_buffOut->samplesLeftGet() < 1) && (m_buffOut->samplesLeftPut() < 1))
			{
				m_buffOut->reset();
			}
		}
	}

	outputSize = int64_t(outputPos);

	if(outputSize < inputSize)
	{
		m_delayedSamples += (inputSize - outputSize);
	}

	if(inputSamplesLeft > 0)
	{
		LOG_WRN("No all input samples could be processed -> discarding pending input!");
		return false;
	}

	return true;
}

bool MDynamicAudioNormalizer::flushBuffer(double **samplesOut, const int64_t bufferSize, int64_t &outputSize)
{
	try
	{
		return p->flushBuffer(samplesOut, bufferSize, outputSize);
	}
	catch(std::exception &e)
	{
		LOG_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::flushBuffer(double **samplesOut, const int64_t bufferSize, int64_t &outputSize)
{
	outputSize = 0;

	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}

	m_flushBuffer = true;
	const int64_t pendingSamples = std::min(std::min(std::max(bufferSize, int64_t(0)), m_delayedSamples), int64_t(UINT32_MAX));

	if(pendingSamples < 1)
	{
		return true; /*no pending samples left*/
	}

	for(uint32_t c = 0; c < m_channels; c++)
	{
		for(uint32_t i = 0; i < pendingSamples; i++)
		{
			samplesOut[c][i] = m_peakValue;
		}
	}

	const bool success = processInplace(samplesOut, pendingSamples, outputSize, true);

	if(success)
	{
		m_delayedSamples -= outputSize;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

void MDynamicAudioNormalizer::getVersionInfo(uint32_t &major, uint32_t &minor,uint32_t &patch)
{
	major = DYNAUDNORM_VERSION_MAJOR;
	minor = DYNAUDNORM_VERSION_MINOR;
	patch = DYNAUDNORM_VERSION_PATCH;
}

void MDynamicAudioNormalizer::getBuildInfo(const char **date, const char **time, const char **compiler, const char **arch, bool &debug)
{
	*date     = DYNAUDNORM_BUILD_DATE;
	*time     = DYNAUDNORM_BUILD_TIME;
	*compiler = DYNAUDNORM_COMPILER;
	*arch     = DYNAUDNORM_ARCH;
	debug     = bool(DYNAUDNORM_DEBUG);
}

MDynamicAudioNormalizer::LogFunction *MDynamicAudioNormalizer::setLogFunction(MDynamicAudioNormalizer::LogFunction *const logFunction)
{
	return DYNAUDNORM_LOG_SETCALLBACK(logFunction);
}

///////////////////////////////////////////////////////////////////////////////
// Procesing Functions
///////////////////////////////////////////////////////////////////////////////

void MDynamicAudioNormalizer_PrivateData::analyzeFrame(FrameData *frame)
{
	//Perform DC Correction (optional)
	if(m_enableDCCorrection)
	{
		perfromDCCorrection(frame, m_gainHistory_original[0].empty());
	}

	//Find the frame's peak sample value
	if(m_channelsCoupled)
	{
		const double peakMagnitude = findPeakMagnitude(frame);
		const double currentGainFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
		
		for(uint32_t c = 0; c < m_channels; c++)
		{
			updateGainHistory(c, currentGainFactor);
		}
	}
	else
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			const double peakMagnitude = findPeakMagnitude(frame, c);
			const double currentGainFactor = std::min(m_peakValue / peakMagnitude, m_maxAmplification);
			updateGainHistory(c, currentGainFactor);
		}
	}
}

void MDynamicAudioNormalizer_PrivateData::amplifyFrame(FrameData *frame)
{
	for(uint32_t c = 0; c < m_channels; c++)
	{
		if(m_gainHistory_smoothed[c].empty())
		{
			LOG_WRN("There are no information available for the current frame!");
			break;
		}

		double *const dataPtr = frame->data(c);

		const double currAmplificationFactor = m_gainHistory_smoothed[c].front();
		m_gainHistory_smoothed[c].pop_front();
		const double nextAmplificationFactor = m_gainHistory_smoothed[c].empty() ? currAmplificationFactor : m_gainHistory_smoothed[c].front();

		if(m_logFile)
		{
			fprintf(m_logFile, c ? "\t%.4f" : "%.4f", currAmplificationFactor);
		}

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			const double amplificationFactor = FADE(m_prevAmplificationFactor[c], currAmplificationFactor, nextAmplificationFactor, i, m_fadeFactors);
			dataPtr[i] *= amplificationFactor;
			LOG_VALUE(amplificationFactor, m_prevAmplificationFactor[channel], currAmplificationFactor, nextAmplificationFactor, channel, i);
			if(fabs(dataPtr[i]) > m_peakValue)
			{
				dataPtr[i] = copysign(m_peakValue, dataPtr[i]); /*fix rare clipping*/
			}
		}

		m_prevAmplificationFactor[c] = currAmplificationFactor;
	}

	if(m_logFile)
	{
		fprintf(m_logFile, "\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

double MDynamicAudioNormalizer_PrivateData::findPeakMagnitude(FrameData *frame, const uint32_t channel)
{
	double dMax = DBL_EPSILON;

	if(channel == UINT32_MAX)
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			double *dataPtr = frame->data(c);
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				dMax = std::max(dMax, fabs(dataPtr[i]));
			}
		}
	}
	else
	{
		double *dataPtr = frame->data(channel);
		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			dMax = std::max(dMax, fabs(dataPtr[i]));
		}
	}

	return dMax;
}

void MDynamicAudioNormalizer_PrivateData::updateGainHistory(const uint32_t &channel, const double &currentGainFactor)
{
	//Pre-fill the gain history
	if(m_gainHistory_original[channel].empty() || m_gainHistory_minimum[channel].empty())
	{
		const uint32_t preFillSize = m_filterSize / 2;
		while(m_gainHistory_original[channel].size() < preFillSize)
		{
			m_gainHistory_original[channel].push_back(currentGainFactor);
		}
		while(m_gainHistory_minimum[channel].size() < preFillSize)
		{
			m_gainHistory_minimum[channel].push_back(1.0);
		}
	}

	//Insert current gain factor
	m_gainHistory_original[channel].push_back(currentGainFactor);

	//Apply the minimum filter
	while(m_gainHistory_original[channel].size() >= m_filterSize)
	{
		assert(m_gainHistory_original[channel].size() == m_filterSize);
		const double minimum = m_minimumFilter->apply(m_gainHistory_original[channel]);

		m_gainHistory_minimum[channel].push_back(minimum);
		m_gainHistory_original[channel].pop_front();
	}

	//Apply the Gaussian filter
	while(m_gainHistory_minimum[channel].size() >= m_filterSize)
	{
		assert(m_gainHistory_minimum[channel].size() == m_filterSize);
		const double smoothed = m_gaussianFilter->apply(m_gainHistory_minimum[channel]);
			
		m_gainHistory_smoothed[channel].push_back(smoothed);
		m_gainHistory_minimum[channel].pop_front();
	}
}

void MDynamicAudioNormalizer_PrivateData::perfromDCCorrection(FrameData *frame, const bool &isFirstFrame)
{
	for(uint32_t c = 0; c < m_channels; c++)
	{
		double *const dataPtr = frame->data(c);
		const double diff = 1.0 / double(m_frameLen);
		double currentAverageValue = 0.0;

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			currentAverageValue += (dataPtr[i] * diff);
		}

		const double prevCorrectionValue = m_dcCorrectionValue[c];
		m_dcCorrectionValue[c] = isFirstFrame ? currentAverageValue : UPDATE_VALUE(currentAverageValue, m_dcCorrectionValue[c], 0.1);

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			dataPtr[i] -= FADE(prevCorrectionValue, m_dcCorrectionValue[c], m_dcCorrectionValue[c], i, m_fadeFactors);
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
