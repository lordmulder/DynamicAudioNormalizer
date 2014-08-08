//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
//////////////////////////////////////////////////////////////////////////////////

#define __STDC_LIMIT_MACROS

#include "AudioIO_Raw.h"

#include "Common.h"

#include <stdexcept>
#include <algorithm>

#define MY_THROW(X) throw std::runtime_error((X))

#define ACCESS_RD 1
#define ACCESS_WR 2

template<typename T> static inline T LIMIT(const T &min, const T &val, const T &max)
{
	return std::min(max, std::max(min, val));
}

static inline double DEC_SAMPLE(const int8_t  &val) { return LIMIT(-1.0, double(val) / double(INT8_MAX ), 1.0); }
static inline double DEC_SAMPLE(const int16_t &val) { return LIMIT(-1.0, double(val) / double(INT16_MAX), 1.0); }
static inline double DEC_SAMPLE(const float   &val) { return LIMIT(-1.0, double(val), 1.0); }

static inline int8_t  ENC_SAMPLE_8 (const double &val) { return static_cast<int8_t >(LIMIT(-1.0, val, 1.0) * double(INT8_MAX )); }
static inline int16_t ENC_SAMPLE_16(const double &val) { return static_cast<int16_t>(LIMIT(-1.0, val, 1.0) * double(INT16_MAX)); }
static inline float   ENC_SAMPLE_32(const double &val) { return static_cast<float  >(LIMIT(-1.0, val, 1.0)); }

///////////////////////////////////////////////////////////////////////////////
// Private Data
///////////////////////////////////////////////////////////////////////////////

class AudioIO_Raw_Private
{
public:
	AudioIO_Raw_Private(void);
	~AudioIO_Raw_Private(void);

	//Open and Close
	bool openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth);
	bool openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth);
	bool close(void);

	//Read and Write
	int64_t read(double **buffer, const int64_t count);
	int64_t write(double *const *buffer, const int64_t count);

	//Query info
	bool queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth);
	void getFormatInfo(CHR *buffer, const uint32_t buffSize);

private:
	//File handle
	FILE*file;
	int access;

	//Properties
	struct
	{
		uint32_t channels;
		uint32_t sampleRate;
		uint32_t bitDepth;
	}
	info;

	//(De)Interleaving buffer
	void* tempBuff;
	static const size_t BUFF_SIZE = 2048;

	//Library info
	static char versionBuffer[128];

	//Helper functions
	static bool checkProperties(const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

AudioIO_Raw::AudioIO_Raw(void)
:
	p(new AudioIO_Raw_Private())
{
}

AudioIO_Raw::~AudioIO_Raw(void)
{
	delete p;
}

AudioIO_Raw_Private::AudioIO_Raw_Private(void)
{
	access = 0;
	file = NULL;
	tempBuff = NULL;
}

AudioIO_Raw_Private::~AudioIO_Raw_Private(void)
{
	close();
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_Raw::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	return p->openRd(fileName, channels, sampleRate, bitDepth);
}

bool AudioIO_Raw::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	return p->openWr(fileName, channels, sampleRate, bitDepth);
}

bool AudioIO_Raw::close(void)
{
	return p->close();
}

int64_t AudioIO_Raw::read(double **buffer, const int64_t count)
{
	return p->read(buffer, count);
}

int64_t AudioIO_Raw::write(double *const *buffer, const int64_t count)
{
	return p->write(buffer, count);
}

bool AudioIO_Raw::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	return p->queryInfo(channels, sampleRate, length, bitDepth);
}

void AudioIO_Raw::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	p->getFormatInfo(buffer, buffSize);
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_Raw_Private::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if(file)
	{
		PRINT_ERR(TXT("Sound file is already open!\n"));
		return false;
	}
	
	if(!checkProperties(channels, sampleRate, bitDepth))
	{
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(info));

	//Open file
	file = STRCASECMP(fileName, TXT("-")) ? FOPEN(fileName, TXT("rb")) : stdin;
	if(!file)
	{
		return false;
	}

	//Store properties
	info.channels   = channels;
	info.sampleRate = sampleRate;
	info.bitDepth   = bitDepth;

	//Allocate temp buffer
	tempBuff = malloc(BUFF_SIZE * (info.bitDepth / 8) * channels);

	access = ACCESS_RD;
	return true;
}

bool AudioIO_Raw_Private::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if(file)
	{
		PRINT_ERR(TXT("Sound file is already open!\n"));
		return false;
	}
	
	if(!checkProperties(channels, sampleRate, bitDepth))
	{
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(info));

	//Open file
	file = file = STRCASECMP(fileName, TXT("-")) ? FOPEN(fileName, TXT("wb")) : stdout;
	if(!file)
	{
		return false;
	}

	//Store properties
	info.channels   = channels;
	info.sampleRate = sampleRate;
	info.bitDepth   = bitDepth;

	//Allocate temp buffer
	tempBuff = malloc(BUFF_SIZE * (info.bitDepth / 8) * channels);

	access = ACCESS_WR;
	return true;
}

bool AudioIO_Raw_Private::close(void)
{
	bool result = false;

	//Close file
	if(file)
	{
		if((file != stdin) && (file != stdout) && (file != stderr))
		{
			fclose(file);
		}
		file = NULL;
	}

	//Free temp buffer
	MY_FREE(tempBuff);

	//Clear status
	access = 0;
	memset(&info, 0, sizeof(info));
	
	return result;
}

int64_t AudioIO_Raw_Private::read(double **buffer, const int64_t count)
{
	if(!file)
	{
		MY_THROW("Audio file not currently open!");
	}
	if(access != ACCESS_RD)
	{
		MY_THROW("Audio file not open in READ mode!");
	}

	int64_t offset    = 0;
	int64_t remaining = count;

	while(remaining > 0)
	{
		//Read data
		const int64_t rdSize = std::min(remaining, int64_t(BUFF_SIZE));
		const int64_t result = fread(tempBuff, (info.bitDepth / 8) * info.channels, size_t(rdSize), file);

		//Deinterleaving
		for(int64_t i = 0; i < result; i++)
		{
			for(uint32_t c = 0; c < info.channels; c++)
			{
				switch(info.bitDepth)
				{
				case 8:
					buffer[c][i + offset] = DEC_SAMPLE(reinterpret_cast<int8_t*> (tempBuff)[(i * info.channels) + c]);
					break;
				case 16:
					buffer[c][i + offset] = DEC_SAMPLE(reinterpret_cast<int16_t*>(tempBuff)[(i * info.channels) + c]);
					break;
				case 32:
					buffer[c][i + offset] = DEC_SAMPLE(reinterpret_cast<float*>  (tempBuff)[(i * info.channels) + c]);
					break;
				}
			}
		}

		offset    += result;
		remaining -= result;

		if(result < rdSize)
		{
			if(!feof(file))
			{
				PRINT_WRN(TXT("File read error. Read fewer frames that what was requested!"));
			}
			break;
		}
	}

	//Zero remaining data
	if(remaining > 0)
	{
		for(uint32_t c = 0; c < info.channels; c++)
		{
			memset(&buffer[c][offset], 0, sizeof(double) * size_t(remaining));
		}
	}

	return count - remaining;
}

int64_t AudioIO_Raw_Private::write(double *const *buffer, const int64_t count)
{
	if(!file)
	{
		MY_THROW("Audio file not currently open!");
	}
	if(access != ACCESS_WR)
	{
		MY_THROW("Audio file not open in WRITE mode!");
	}

	int64_t offset    = 0;
	int64_t remaining = count;

	while(remaining > 0)
	{
		const int64_t wrSize = std::min(remaining, int64_t(BUFF_SIZE));

		//Interleave data
		for(int64_t i = 0; i < wrSize; i++)
		{
			for(uint32_t c = 0; c < info.channels; c++)
			{
				switch(info.bitDepth)
				{
				case 8:
					reinterpret_cast<int8_t*> (tempBuff)[(i * info.channels) + c] = ENC_SAMPLE_8 (buffer[c][i + offset]);
					break;
				case 16:
					reinterpret_cast<int16_t*>(tempBuff)[(i * info.channels) + c] = ENC_SAMPLE_16(buffer[c][i + offset]);
					break;
				case 32:
					reinterpret_cast<float*>  (tempBuff)[(i * info.channels) + c] = ENC_SAMPLE_32(buffer[c][i + offset]);
					break;
				}
			}
		}

		//Write data
		const int64_t result = fwrite(tempBuff, (info.bitDepth / 8) * info.channels, size_t(wrSize), file);

		offset    += result;
		remaining -= result;

		if(result < wrSize)
		{
			PRINT_WRN(TXT("File write error. Wrote fewer frames that what was requested!"));
			break;
		}
	}
	
	return count - remaining;
}

bool AudioIO_Raw_Private::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	if(!file)
	{
		MY_THROW("Audio file not currently open!");
	}

	channels   = info.channels;
	sampleRate = info.sampleRate;
	length     = INT64_MAX;
	bitDepth   = info.bitDepth;

	return true;
}

void AudioIO_Raw_Private::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	const CHR *subfmt = TXT("Unknown SubFmt");

	switch(info.bitDepth)
	{
		case 8 : subfmt = TXT("8-Bit Signed");          break;
		case 16: subfmt = TXT("16-Bit Integer");        break;
		case 32: subfmt = TXT("32-Bit Floating-Point"); break;
	}

	SNPRINTF(buffer, buffSize, TXT("Raw Data, %s"), subfmt);
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_Raw_Private::checkProperties(const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if((channels < 1) || (channels > 16))
	{
		PRINT2_ERR(TXT("Invalid number of channels (%u). Must be in the %u to %u range!\n"), channels, 1, 16);
		return false;
	}

	if((sampleRate < 11025) || (sampleRate > 192000))
	{
		PRINT2_ERR(TXT("Invalid sampele rate (%u). Must be in the %u to %u range!\n"), sampleRate, 11025, 192000);
		return false;
	}

	if((bitDepth != 8) && (bitDepth != 16) && (bitDepth != 32))
	{
		PRINT2_ERR(TXT("Invalid bit depth (%u). Only 8, 16 and 32 bits/sample are currently supported!\n"), bitDepth);
		return false;
	}

	return true;
}
