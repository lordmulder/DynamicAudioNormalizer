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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include "DynamicAudioNormalizer.h"

#include "Common.h"
#include "FrameBuffer.h"
#include "MinimumFilter.h"
#include "GaussianFilter.h"
#include "Logging.h"
#include "Version.h"

#include <cmath>
#include <algorithm>
#include <queue>
#include <deque>
#include <cassert>
#include <stdexcept> 
#include <cfloat>
#include <inttypes.h>

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

template<typename T> static inline T LIMIT(const T &min, const T &val, const T &max)
{
	return std::min(max, std::max(min, val));
}

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

static inline double FADE(const double &prev, const double &next, const uint32_t &pos, const double *const fadeFactors[2])
{
	return (fadeFactors[0][pos] * prev) + (fadeFactors[1][pos] * next);
}

static inline double BOUND(const double &threshold, const double &val)
{
	const double CONST = 0.8862269254527580136490837416705725913987747280611935; //sqrt(PI) / 2.0
	return erf(CONST * (val / threshold)) * threshold;
}

static inline double POW2(const double &value)
{
	return value * value;
}

#define BOOLIFY(X) ((X) ? "YES" : "NO")
#define POW2(X) ((X)*(X))

#define CLEAR_QUEUE(X) do \
{ \
	while(!(X).empty()) (X).pop(); \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// MDynamicAudioNormalizer_PrivateData
///////////////////////////////////////////////////////////////////////////////

class MDynamicAudioNormalizer_PrivateData
{
public:
	MDynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const double compressThresh, const bool channelsCoupled, const bool enableDCCorrection, const bool altBoundaryMode, FILE *const logFile);
	~MDynamicAudioNormalizer_PrivateData(void);

	bool initialize(void);
	bool processInplace(double **samplesInOut, const int64_t inputSize, int64_t &outputSize, const bool &bFlush);
	bool flushBuffer(double **samplesOut, const int64_t bufferSize, int64_t &outputSize);
	bool reset(void);
	bool getConfiguration(uint32_t &channels, uint32_t &sampleRate, uint32_t &frameLen, uint32_t &filterSize);
	bool getInternalDelay(int64_t &delayInSamples);

private:
	const uint32_t m_channels;
	const uint32_t m_sampleRate;
	const uint32_t m_frameLen;
	const uint32_t m_filterSize;
	const uint32_t m_delay;

	const double m_peakValue;
	const double m_maxAmplification;
	const double m_targetRms;
	const double m_compressFactor;

	const bool m_channelsCoupled;
	const bool m_enableDCCorrection;
	const bool m_altBoundaryMode;

	FILE *const m_logFile;
	
	bool m_initialized;
	bool m_flushBuffer;

	FrameFIFO *m_buffSrc;
	FrameFIFO *m_buffOut;
	
	int64_t m_delayedSamples;

	uint64_t m_sampleCounterTotal;
	uint64_t m_sampleCounterClips;
	uint64_t m_sampleCounterCompr;

	FrameBuffer *m_frameBuffer;
	
	std::deque<double> *m_gainHistory_original;
	std::deque<double> *m_gainHistory_minimum;
	std::deque<double> *m_gainHistory_smoothed;

	std::queue<double> *m_loggingData_original;
	std::queue<double> *m_loggingData_minimum;
	std::queue<double> *m_loggingData_smoothed;

	MinimumFilter  *m_minimumFilter;
	GaussianFilter *m_gaussianFilter;

	double *m_prevAmplificationFactor;
	double *m_dcCorrectionValue;
	double *m_compressThreshold;

	double *m_fadeFactors[2];

protected:
	void processNextFrame(void);
	void analyzeFrame(FrameData *frame);
	void amplifyFrame(FrameData *frame);
	
	double getMaxLocalGain(FrameData *frame, const uint32_t channel = UINT32_MAX);
	double findPeakMagnitude(FrameData *frame, const uint32_t channel = UINT32_MAX);
	double computeFrameRMS(const FrameData *frame, const uint32_t channel = UINT32_MAX);
	double computeFrameStdDev(const FrameData *frame, const uint32_t channel = UINT32_MAX);
	void updateGainHistory(const uint32_t &channel, const double &currentGainFactor);
	void perfromDCCorrection(FrameData *frame, const bool &isFirstFrame);
	void perfromCompression(FrameData *frame, const bool &isFirstFrame);
	void writeLogFile(void);
	void printParameters(void);
	
	static void precalculateFadeFactors(double *fadeFactors[2], const uint32_t frameLen);
	static double setupCompressThresh(const double &dThreshold);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

MDynamicAudioNormalizer::MDynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const double compressThresh, const bool channelsCoupled, const bool enableDCCorrection, const bool altBoundaryMode, FILE *const logFile)
:
	p(new MDynamicAudioNormalizer_PrivateData(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressThresh, channelsCoupled, enableDCCorrection, altBoundaryMode,logFile))
{
	/*nothing to do here*/
}

MDynamicAudioNormalizer_PrivateData::MDynamicAudioNormalizer_PrivateData(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec, const uint32_t filterSize, const double peakValue, const double maxAmplification, const double targetRms, const double compressFactor, const bool channelsCoupled, const bool enableDCCorrection, const bool altBoundaryMode, FILE *const logFile)
:
	m_channels(channels),
	m_sampleRate(sampleRate),
	m_frameLen(FRAME_SIZE(sampleRate, frameLenMsec)),
	m_filterSize(LIMIT(3u, filterSize, 301u)),
	m_delay(m_frameLen * m_filterSize),
	m_peakValue(LIMIT(0.01, peakValue, 1.0)),
	m_maxAmplification(LIMIT(1.0, maxAmplification, 100.0)),
	m_targetRms(LIMIT(0.0, targetRms, 1.0)),
	m_compressFactor(compressFactor ? LIMIT(1.0, compressFactor, 30.0) : 0.0),
	m_channelsCoupled(channelsCoupled),
	m_enableDCCorrection(enableDCCorrection),
	m_altBoundaryMode(altBoundaryMode),
	m_logFile(logFile)
{
	// LOG2_DBG("channels: %u",           channels);
	// LOG2_DBG("sampleRate: %u",         sampleRate);
	// LOG2_DBG("frameLenMsec: %u",       frameLenMsec);
	// LOG2_DBG("filterSize: %u",         filterSize);
	// LOG2_DBG("peakValue: %.4f",        peakValue);
	// LOG2_DBG("maxAmplification: %.4f", maxAmplification);
	// LOG2_DBG("targetRms: %.4f",        targetRms);
	// LOG2_DBG("compressFactor: %.4f",   compressFactor);
	// LOG2_DBG("channelsCoupled: %s",    channelsCoupled    ? "YES" : "NO");
	// LOG2_DBG("enableDCCorrection: %s", enableDCCorrection ? "YES" : "NO");
	// LOG2_DBG("altBoundaryMode: %s",    altBoundaryMode    ? "YES" : "NO");
	// LOG2_DBG("logFile: %p",            logFile);
	
	m_initialized = false;
	m_flushBuffer = false;

	m_buffSrc = NULL;
	m_buffOut = NULL;
	
	m_delayedSamples = 0;
	m_sampleCounterClips = m_sampleCounterTotal = 0;
	m_frameBuffer = NULL;

	m_gainHistory_original = NULL;
	m_gainHistory_minimum  = NULL;
	m_gainHistory_smoothed = NULL;

	m_loggingData_original = NULL;
	m_loggingData_minimum  = NULL;
	m_loggingData_smoothed = NULL;

	m_minimumFilter  = NULL;
	m_gaussianFilter = NULL;

	m_prevAmplificationFactor = NULL;
	m_dcCorrectionValue = NULL;
	m_compressThreshold = NULL;

	m_fadeFactors[0] = m_fadeFactors[1] = NULL;
}

MDynamicAudioNormalizer::~MDynamicAudioNormalizer(void)
{
	delete p;
}

MDynamicAudioNormalizer_PrivateData::~MDynamicAudioNormalizer_PrivateData(void)
{
	LOG2_DBG("Processed %" PRIu64 " samples total, clipped %" PRIu64 " samples (%.2f%%).",
		m_sampleCounterTotal,
		m_sampleCounterClips,
		m_sampleCounterTotal ? (double(m_sampleCounterClips) / double(m_sampleCounterTotal) * 100.0) : 0.0
	);

	MY_DELETE(m_buffSrc);
	MY_DELETE(m_buffOut);

	MY_DELETE(m_frameBuffer);

	MY_DELETE(m_minimumFilter);
	MY_DELETE(m_gaussianFilter);
	
	MY_DELETE_ARRAY(m_gainHistory_original);
	MY_DELETE_ARRAY(m_gainHistory_minimum );
	MY_DELETE_ARRAY(m_gainHistory_smoothed);

	MY_DELETE_ARRAY(m_loggingData_original);
	MY_DELETE_ARRAY(m_loggingData_minimum );
	MY_DELETE_ARRAY(m_loggingData_smoothed);

	MY_DELETE_ARRAY(m_prevAmplificationFactor);
	MY_DELETE_ARRAY(m_dcCorrectionValue);
	MY_DELETE_ARRAY(m_compressThreshold);

	MY_DELETE_ARRAY(m_fadeFactors[0]);
	MY_DELETE_ARRAY(m_fadeFactors[1]);
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
		LOG1_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::initialize(void)
{
	if(m_initialized)
	{
		LOG1_WRN("Already initialized -> ignoring!");
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
	if(m_logFile && (ferror(m_logFile) != 0))
	{
		LOG1_WRN("Specified log file has the error indicator set, no logging information will be created!");
	}

	m_buffSrc = new FrameFIFO(m_channels, m_frameLen);
	m_buffOut = new FrameFIFO(m_channels, m_frameLen);

	m_frameBuffer = new FrameBuffer(m_channels, m_frameLen, m_filterSize + 1);

	m_gainHistory_original = new std::deque<double>[m_channels];
	m_gainHistory_minimum  = new std::deque<double>[m_channels];
	m_gainHistory_smoothed = new std::deque<double>[m_channels];

	m_loggingData_original = new std::queue<double>[m_channels];
	m_loggingData_minimum  = new std::queue<double>[m_channels];
	m_loggingData_smoothed = new std::queue<double>[m_channels];

	m_minimumFilter = new MinimumFilter(m_filterSize);
	const double sigma = (((double(m_filterSize) / 2.0) - 1.0) / 3.0) + (1.0 / 3.0);
	m_gaussianFilter = new GaussianFilter(m_filterSize, sigma);

	m_dcCorrectionValue       = new double[m_channels];
	m_prevAmplificationFactor = new double[m_channels];
	m_compressThreshold       = new double[m_channels];

	m_fadeFactors[0] = new double[m_frameLen];
	m_fadeFactors[1] = new double[m_frameLen];
	
	precalculateFadeFactors(m_fadeFactors, m_frameLen);
	printParameters();

	if(m_logFile)
	{
		fprintf(m_logFile, "DynamicAudioNormalizer Logfile v%u.%02u-%u\n", DYNAUDNORM_VERSION_MAJOR, DYNAUDNORM_VERSION_MINOR, DYNAUDNORM_VERSION_PATCH);
		fprintf(m_logFile, "CHANNEL_COUNT:%u\n\n", m_channels);
	}

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
		LOG1_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::reset(void)
{
	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG1_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}

	m_delayedSamples = 0;
	m_flushBuffer = false;

	m_buffSrc->reset();
	m_buffOut->reset();

	m_frameBuffer->reset();

	for(uint32_t c = 0; c < m_channels; c++)
	{
		m_gainHistory_original[c].clear();
		m_gainHistory_minimum [c].clear();
		m_gainHistory_smoothed[c].clear();

		CLEAR_QUEUE(m_loggingData_original[c]);
		CLEAR_QUEUE(m_loggingData_minimum [c]);
		CLEAR_QUEUE(m_loggingData_smoothed[c]);

		m_dcCorrectionValue      [c] = 0.0;
		m_prevAmplificationFactor[c] = 1.0;
		m_compressThreshold      [c] = 0.0;
	}

	return true;
}

bool MDynamicAudioNormalizer::getConfiguration(uint32_t &channels, uint32_t &sampleRate, uint32_t &frameLen, uint32_t &filterSize)
{
	try
	{
		return p->getConfiguration(channels, sampleRate, frameLen, filterSize);
	}
	catch(std::exception &e)
	{
		LOG1_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer::getInternalDelay(int64_t &delayInSamples)
{
	try
	{
		return p->getInternalDelay(delayInSamples);
	}
	catch(std::exception &e)
	{
		LOG1_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::getConfiguration(uint32_t &channels, uint32_t &sampleRate, uint32_t &frameLen, uint32_t &filterSize)
{
	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG1_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}

	channels = m_channels;
	sampleRate = m_sampleRate;
	frameLen = m_frameLen;
	filterSize = m_filterSize;

	return true;
}

bool MDynamicAudioNormalizer_PrivateData::getInternalDelay(int64_t &delayInSamples)
{
	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG1_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}

	delayInSamples = m_delay; //m_frameLen * m_filterSize;
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
		LOG1_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::processInplace(double **samplesInOut, const int64_t inputSize, int64_t &outputSize, const bool &bFlush)
{
	outputSize = 0;

	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG1_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}
	if(m_flushBuffer && (!bFlush))
	{
		LOG1_ERR("Must not call processInplace() after flushBuffer(). Call reset() first!");
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
			m_delayedSamples += copyLen;
		}

		//Analyze next input frame, if we have enough input
		if(m_buffSrc->samplesLeftGet() >= m_frameLen)
		{
			bStop = false;
			analyzeFrame(m_buffSrc->data());
			
			if(!m_frameBuffer->putFrame(m_buffSrc))
			{
				LOG1_ERR("Failed to append current input frame to internal buffer!");
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
				LOG1_ERR("Failed to retrieve next output frame from internal buffer!");
				return false;
			}

			amplifyFrame(m_buffOut->data());
		}

		//Write as many output samples as possible
		while((outputBufferLeft > 0) && (m_buffOut->samplesLeftGet() > 0) && (bFlush || (m_delayedSamples > m_delay)))
		{
			bStop = false;

			const uint32_t pending = bFlush ? UINT32_MAX : uint32_t(m_delayedSamples - m_delay);
			const uint32_t copyLen = std::min(std::min(outputBufferLeft, m_buffOut->samplesLeftGet()), pending);
			m_buffOut->getSamples(samplesInOut, outputPos, copyLen);

			outputPos        += copyLen;
			outputBufferLeft -= copyLen;
			m_delayedSamples -= copyLen;

			if((m_buffOut->samplesLeftGet() < 1) && (m_buffOut->samplesLeftPut() < 1))
			{
				m_buffOut->reset();
			}
		}
	}

	outputSize = int64_t(outputPos);

	if(inputSamplesLeft > 0)
	{
		LOG1_WRN("No all input samples could be processed -> discarding pending input!");
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
		LOG1_ERR(e.what());
		return false;
	}
}

bool MDynamicAudioNormalizer_PrivateData::flushBuffer(double **samplesOut, const int64_t bufferSize, int64_t &outputSize)
{
	outputSize = 0;

	//Check audio normalizer status
	if(!m_initialized)
	{
		LOG1_ERR("Not initialized yet. Must call initialize() first!");
		return false;
	}

	m_flushBuffer = true;
	const int64_t pendingSamples = std::min(std::min(std::max(bufferSize, int64_t(0)), m_delayedSamples), int64_t(UINT32_MAX));

	if(pendingSamples < 1)
	{
		return true; /*no pending samples left*/
	}

	bool success = false;
	do
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			for(uint32_t i = 0; i < pendingSamples; i++)
			{
				samplesOut[c][i] = m_altBoundaryMode ? DBL_EPSILON : ((m_targetRms > DBL_EPSILON) ? std::min(m_peakValue, m_targetRms) : m_peakValue);
				if(m_enableDCCorrection)
				{
					samplesOut[c][i] *= ((i % 2) == 1) ? (-1) : 1;
					samplesOut[c][i] += m_dcCorrectionValue[c];
				}
			}
		}

		success = processInplace(samplesOut, pendingSamples, outputSize, true);
	}
	while(success && (outputSize <= 0));

	if(success && (outputSize > 0))
	{
		m_delayedSamples -= outputSize;
	}
	
	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Public Static Functions
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

	//Perform compression (optional)
	if(m_compressFactor > DBL_EPSILON)
	{
		perfromCompression(frame, m_gainHistory_original[0].empty());
	}

	//Find the frame's peak sample value
	if(m_channelsCoupled)
	{
		const double currentGainFactor = getMaxLocalGain(frame);
		for(uint32_t c = 0; c < m_channels; c++)
		{
			updateGainHistory(c, currentGainFactor);
		}
	}
	else
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			updateGainHistory(c, getMaxLocalGain(frame, c));
		}
	}

	//Write data to the log file
	writeLogFile();
}

void MDynamicAudioNormalizer_PrivateData::amplifyFrame(FrameData *frame)
{
	for(uint32_t c = 0; c < m_channels; c++)
	{
		if(m_gainHistory_smoothed[c].empty())
		{
			LOG1_WRN("There are no information available for the current frame!");
			break;
		}

		double *const dataPtr = frame->data(c);
		const double currAmplificationFactor = m_gainHistory_smoothed[c].front();
		m_gainHistory_smoothed[c].pop_front();

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			const double amplificationFactor = FADE(m_prevAmplificationFactor[c], currAmplificationFactor, i, m_fadeFactors);
			dataPtr[i] *= amplificationFactor;
			LOG_VALUE(amplificationFactor, m_prevAmplificationFactor[c], currAmplificationFactor, nextAmplificationFactor, c, i);
			if(fabs(dataPtr[i]) > m_peakValue)
			{
				m_sampleCounterClips++;
				dataPtr[i] = copysign(m_peakValue, dataPtr[i]); /*fix rare clipping*/
			}
		}
		
		m_prevAmplificationFactor[c] = currAmplificationFactor;
		m_sampleCounterTotal += m_frameLen;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

double MDynamicAudioNormalizer_PrivateData::getMaxLocalGain(FrameData *frame, const uint32_t channel)
{
	const double maximumGain = m_peakValue / findPeakMagnitude(frame, channel);
	const double rmsGain = (m_targetRms > DBL_EPSILON) ? (m_targetRms / computeFrameRMS(frame, channel)) : DBL_MAX;
	return BOUND(m_maxAmplification, std::min(maximumGain, rmsGain));
}

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

double MDynamicAudioNormalizer_PrivateData::computeFrameRMS(const FrameData *frame, const uint32_t channel)
{
	double rmsValue = 0.0;
	
	if(channel == UINT32_MAX)
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			const double *dataPtr = frame->data(c);
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				rmsValue += POW2(dataPtr[i]);
			}
		}
		rmsValue /= double(m_frameLen * m_channels);
	}
	else
	{
		const double *dataPtr = frame->data(channel);
		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			rmsValue += POW2(dataPtr[i]);
		}
		rmsValue /= double(m_frameLen);
	}

	return std::max(sqrt(rmsValue), DBL_EPSILON);
}

double MDynamicAudioNormalizer_PrivateData::computeFrameStdDev(const FrameData *frame, const uint32_t channel)
{
	double variance = 0.0;

	if(channel == UINT32_MAX)
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			const double *dataPtr = frame->data(c);
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				variance += POW2(dataPtr[i]);	//Assume that MEAN is *zero*
			}
		}
		variance /= double((m_channels * m_frameLen) - 1);
	}
	else
	{
		const double *dataPtr = frame->data(channel);
		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			variance += POW2(dataPtr[i]);	//Assume that MEAN is *zero*
		}
		variance /= double(m_frameLen - 1);
	}


	return std::max(sqrt(variance), DBL_EPSILON);
}

void MDynamicAudioNormalizer_PrivateData::updateGainHistory(const uint32_t &channel, const double &currentGainFactor)
{
	//Pre-fill the gain history
	if(m_gainHistory_original[channel].empty() || m_gainHistory_minimum[channel].empty())
	{
		const uint32_t preFillSize = m_filterSize / 2;
		m_prevAmplificationFactor[channel] = m_altBoundaryMode ? currentGainFactor : 1.0;
		while(m_gainHistory_original[channel].size() < preFillSize)
		{
			m_gainHistory_original[channel].push_back(m_altBoundaryMode ? currentGainFactor : 1.0);
		}
		while(m_gainHistory_minimum[channel].size() < preFillSize)
		{
			m_gainHistory_minimum[channel].push_back(m_altBoundaryMode ? currentGainFactor : 1.0);
		}
	}

	//Insert current gain factor
	m_gainHistory_original[channel].push_back(currentGainFactor);
	m_loggingData_original[channel].push     (currentGainFactor);

	//Apply the minimum filter
	while(m_gainHistory_original[channel].size() >= m_filterSize)
	{
		assert(m_gainHistory_original[channel].size() == m_filterSize);
		const double minimum = m_minimumFilter->apply(m_gainHistory_original[channel]);

		m_gainHistory_minimum[channel].push_back(minimum);
		m_loggingData_minimum[channel].push     (minimum);

		m_gainHistory_original[channel].pop_front();
	}

	//Apply the Gaussian filter
	while(m_gainHistory_minimum[channel].size() >= m_filterSize)
	{
		assert(m_gainHistory_minimum[channel].size() == m_filterSize);
		const double smoothed = m_gaussianFilter->apply(m_gainHistory_minimum[channel]);
			
		m_gainHistory_smoothed[channel].push_back(smoothed);
		m_loggingData_smoothed[channel].push     (smoothed);

		m_gainHistory_minimum[channel].pop_front();
	}
}

void MDynamicAudioNormalizer_PrivateData::perfromDCCorrection(FrameData *frame, const bool &isFirstFrame)
{
	const double diff = 1.0 / double(m_frameLen);

	for(uint32_t c = 0; c < m_channels; c++)
	{
		double *const dataPtr = frame->data(c);
		double currentAverageValue = 0.0;

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			currentAverageValue += (dataPtr[i] * diff);
		}

		const double prevValue = isFirstFrame ? currentAverageValue : m_dcCorrectionValue[c];
		m_dcCorrectionValue[c] = isFirstFrame ? currentAverageValue : UPDATE_VALUE(currentAverageValue, m_dcCorrectionValue[c], 0.1);

		for(uint32_t i = 0; i < m_frameLen; i++)
		{
			dataPtr[i] -= FADE(prevValue, m_dcCorrectionValue[c], i, m_fadeFactors);
		}
	}
}

void MDynamicAudioNormalizer_PrivateData::perfromCompression(FrameData *frame, const bool &isFirstFrame)
{
	if(m_channelsCoupled)
	{
		const double standardDeviation = computeFrameStdDev(frame);
		const double currentThreshold  = std::min(1.0, m_compressFactor * standardDeviation);

		const double prevValue = isFirstFrame ? currentThreshold : m_compressThreshold[0];
		m_compressThreshold[0] = isFirstFrame ? currentThreshold : UPDATE_VALUE(currentThreshold, m_compressThreshold[0], (1.0/3.0));

		const double prevActualThresh = setupCompressThresh(prevValue);
		const double currActualThresh = setupCompressThresh(m_compressThreshold[0]);

		for(uint32_t c = 0; c < m_channels; c++)
		{
			double *const dataPtr = frame->data(c);
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				const double localThresh = FADE(prevActualThresh, currActualThresh, i, m_fadeFactors);
				dataPtr[i] = copysign(BOUND(localThresh, fabs(dataPtr[i])), dataPtr[i]);
			}
		}
	}
	else
	{
		for(uint32_t c = 0; c < m_channels; c++)
		{
			const double standardDeviation = computeFrameStdDev(frame, c);
			const double currentThreshold  = setupCompressThresh(std::min(1.0,m_compressFactor * standardDeviation));

			const double prevValue = isFirstFrame ? currentThreshold : m_compressThreshold[c];
			m_compressThreshold[c] = isFirstFrame ? currentThreshold : UPDATE_VALUE(currentThreshold, m_compressThreshold[c], (1.0/3.0));

			const double prevActualThresh = setupCompressThresh(prevValue);
			const double currActualThresh = setupCompressThresh(m_compressThreshold[c]);

			double *const dataPtr = frame->data(c);
			for(uint32_t i = 0; i < m_frameLen; i++)
			{
				const double localThresh = FADE(prevActualThresh, currActualThresh, i, m_fadeFactors);
				dataPtr[i] = copysign(BOUND(localThresh, fabs(dataPtr[i])), dataPtr[i]);
			}
		}
	}
}

void MDynamicAudioNormalizer_PrivateData::writeLogFile(void)
{
	bool bWritten = false;
		
	for(uint32_t c = 0; c < m_channels; c++)
	{
		if(!(m_loggingData_original[c].empty() || m_loggingData_minimum[c].empty() || m_loggingData_smoothed[c].empty()))
		{
			if(m_logFile && (ferror(m_logFile) == 0))
			{
				if(bWritten)
				{
					fprintf(m_logFile, "\t\t");
				}

				fprintf(m_logFile, "%.5f\t%.5f\t%.5f", m_loggingData_original[c].front(), m_loggingData_minimum[c].front(), m_loggingData_smoothed[c].front());
				
				if(ferror(m_logFile) != 0)
				{
					LOG1_WRN("Error while writing to log file. No further logging output will be created.");
				}
				
				bWritten = true;
			}

			m_loggingData_original[c].pop();
			m_loggingData_minimum [c].pop();
			m_loggingData_smoothed[c].pop();
		}
	}

	if(m_logFile && (ferror(m_logFile) == 0) && bWritten)
	{
		fprintf(m_logFile, "\n");

		if(ferror(m_logFile) != 0)
		{
			LOG1_WRN("Error while writing to log file. No further logging output will be created.");
		}
	}
}

void MDynamicAudioNormalizer_PrivateData::printParameters(void)
{
	LOG1_DBG("------- DynamicAudioNormalizer -------");
	LOG2_DBG("Lib Version          : %u.%02u-%u", DYNAUDNORM_VERSION_MAJOR, DYNAUDNORM_VERSION_MINOR ,DYNAUDNORM_VERSION_PATCH);
	LOG2_DBG("m_channels           : %u",         m_channels);
	LOG2_DBG("m_sampleRate         : %u",         m_sampleRate);
	LOG2_DBG("m_frameLen           : %u",         m_frameLen);
	LOG2_DBG("m_filterSize         : %u",         m_filterSize);
	LOG2_DBG("m_peakValue          : %.4f",       m_peakValue);
	LOG2_DBG("m_maxAmplification   : %.4f",       m_maxAmplification);
	LOG2_DBG("m_targetRms          : %.4f",       m_targetRms);
	LOG2_DBG("m_compressFactor     : %.4f",       m_compressFactor);
	LOG2_DBG("m_channelsCoupled    : %s",         BOOLIFY(m_channelsCoupled));
	LOG2_DBG("m_enableDCCorrection : %s",         BOOLIFY(m_enableDCCorrection));
	LOG2_DBG("m_altBoundaryMode    : %s",         BOOLIFY(m_altBoundaryMode));
	LOG1_DBG("------- DynamicAudioNormalizer -------");
}

///////////////////////////////////////////////////////////////////////////////
// Static Utility Functions
///////////////////////////////////////////////////////////////////////////////

void MDynamicAudioNormalizer_PrivateData::precalculateFadeFactors(double *fadeFactors[2], const uint32_t frameLen)
{
	assert((frameLen > 0) && ((frameLen % 2) == 0));
	const double dStepSize = 1.0 / double(frameLen);

	for(uint32_t pos = 0; pos < frameLen; pos++)
	{
		fadeFactors[0][pos] = 1.0 - (dStepSize * double(pos + 1U));
		fadeFactors[1][pos] = 1.0 - fadeFactors[0][pos];
	}

	//for(uint32_t pos = 0; pos < frameLen; pos++)
	//{
	//	LOG2_DBG("%.8f %.8f %.8f", fadeFactors[0][pos], fadeFactors[1][pos], fadeFactors[0][pos] + fadeFactors[1][pos]);
	//}
}

double MDynamicAudioNormalizer_PrivateData::setupCompressThresh(const double &dThreshold)
{
	if((dThreshold > DBL_EPSILON) && (dThreshold < (1.0 - DBL_EPSILON)))
	{
		double dCurrentThreshold = dThreshold;
		double dStepSize = 1.0;
		while(dStepSize > DBL_EPSILON)
		{
			while((dCurrentThreshold + dStepSize > dCurrentThreshold) && (BOUND(dCurrentThreshold + dStepSize, 1.0) <= dThreshold))
			{
				dCurrentThreshold += dStepSize;
			}
			dStepSize /= 2.0;
		}
		return dCurrentThreshold;
	}
	else
	{
		return dThreshold;
	}
}
