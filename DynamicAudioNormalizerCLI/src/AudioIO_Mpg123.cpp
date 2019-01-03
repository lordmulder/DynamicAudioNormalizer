//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2019 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "AudioIO_Mpg123.h"
#include <Common.h>
#include <Threads.h>

#ifdef _WIN32
#include <mpg123_msvc.h>
#define MPG123OPEN(X,Y) mpg123_topen((X),(Y))
#define MPG123CLOSE(X)  mpg123_tclose((X))
#else
#include <mpg123.h>
#define MPG123OPEN(X,Y) mpg123_open((X),(Y))
#define MPG123CLOSE(X)  mpg123_close((X))
#endif

#include <stdexcept>
#include <algorithm>

#ifdef __unix
#include <unistd.h>
#else
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#define MPG123_DELETE(X) do \
{ \
	if((X)) \
	{ \
		mpg123_delete((X)); \
		(X) = NULL; \
	} \
} \
while(0)

#define MY_THROW(X) throw std::runtime_error((X))

///////////////////////////////////////////////////////////////////////////////
// Const
///////////////////////////////////////////////////////////////////////////////

static const size_t MPG123_HDR_SIZE = 4U;

static const uint32_t MPG123_SRATE[2][4] =
{
	{ 22050, 24000, 16000, 0 }, /*V2*/
	{ 44100, 48000, 32000, 0 }  /*V1*/
};

static const uint32_t MPG123_BITRT[2][4][16] =
{
	{
		{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, /*V2*/
		{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, /*L3*/
		{ 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, /*L2*/
		{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }, /*L1*/
	},
	{
		{ 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, /*V1*/
		{ 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 }, /*L3*/
		{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 }, /*L2*/
		{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }, /*L1*/
	}
};

static const uint32_t MPG123_FSIZE[2][4] =
{
	{ 0,  72, 144, 48 }, /*V2*/
	{ 0, 144, 144, 48 }  /*V1*/
};

///////////////////////////////////////////////////////////////////////////////
// FMT123.H COMPAT
///////////////////////////////////////////////////////////////////////////////

#ifndef MPG123_ENC_H
#define MPG123_SAMPLESIZE(enc) ((enc) & MPG123_ENC_8 ? 1 : ((enc) & \
   MPG123_ENC_16 ? 2 : ((enc) & MPG123_ENC_24 ? 3 : (((enc) & MPG123_ENC_32 || \
  (enc) == MPG123_ENC_FLOAT_32) ? 4 : ((enc) == MPG123_ENC_FLOAT_64 ? 8 : 0)))))
struct mpg123_fmt
{
	long rate;
	int channels;
	int encoding;
};
#endif //MPG123_ENC_H

///////////////////////////////////////////////////////////////////////////////
// Private Data
///////////////////////////////////////////////////////////////////////////////

class AudioIO_Mpg123_Private
{
public:
	AudioIO_Mpg123_Private(void);
	~AudioIO_Mpg123_Private(void);

	//Open and Close
	bool openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth);
	bool openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format);
	bool close(void);

	//Read and Write
	int64_t read(double **buffer, const int64_t count);
	int64_t write(double *const *buffer, const int64_t count);

	//Query info
	bool queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth);
	void getFormatInfo(CHR *buffer, const uint32_t buffSize);

	//Library info
	static const CHR *libraryVersion(void);
	static const CHR *const *supportedFormats(const CHR **const list, const uint32_t maxLen);
	static bool checkFileType(FILE *const);

private:
	//libmpg123
	mpg123_handle *handle;

	//audio format info
	struct mpg123_fmt format;

	//(De)Interleaving buffer
	uint8_t *tempBuff;
	size_t frameSize, buffSize;

	//Library info
	static MY_CRITSEC_DECL(mutex_lock);
	static CHR versionBuffer[256];
	static uint32_t instanceCounter;

	//Helper functions
	static uint32_t checkMpg123Sequence(const uint8_t *const buffer, const size_t &count, uint32_t &max_sequence);
	static bool checkMpg123SyncWord(const uint8_t *const buffer, const size_t &fpos);
	template<typename T> static void deinterleave(double **destination, const int64_t offset, const T *const source, const int &channels, const int64_t &len);
	static void libraryInit(void);
	static void libraryExit(void);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

AudioIO_Mpg123::AudioIO_Mpg123(void)
:
	p(new AudioIO_Mpg123_Private())
{
}

AudioIO_Mpg123::~AudioIO_Mpg123(void)
{
	delete p;
}

AudioIO_Mpg123_Private::AudioIO_Mpg123_Private(void)
{
	libraryInit();

	tempBuff  = NULL;
	handle    = NULL;
	frameSize = 0;
	buffSize  = 0;

	memset(&format, 0, sizeof(struct mpg123_fmt));
}

AudioIO_Mpg123_Private::~AudioIO_Mpg123_Private(void)
{
	close();
	libraryExit();
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_Mpg123::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	return p->openRd(fileName, channels, sampleRate, bitDepth);
}

bool AudioIO_Mpg123::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format)
{
	return p->openWr(fileName, channels, sampleRate, bitDepth, format);
}

bool AudioIO_Mpg123::close(void)
{
	return p->close();
}

int64_t AudioIO_Mpg123::read(double **buffer, const int64_t count)
{
	return p->read(buffer, count);
}

int64_t AudioIO_Mpg123::write(double *const *buffer, const int64_t count)
{
	return p->write(buffer, count);
}

bool AudioIO_Mpg123::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	return p->queryInfo(channels, sampleRate, length, bitDepth);
}

void AudioIO_Mpg123::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	p->getFormatInfo(buffer, buffSize);
}

const CHR *AudioIO_Mpg123::libraryVersion(void)
{
	return AudioIO_Mpg123_Private::libraryVersion();
}

const CHR *const *AudioIO_Mpg123::supportedFormats(const CHR **const list, const uint32_t maxLen)
{
	return AudioIO_Mpg123_Private::supportedFormats(list, maxLen);
}

bool AudioIO_Mpg123::checkFileType(FILE *const file)
{
	return AudioIO_Mpg123_Private::checkFileType(file);
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_Mpg123_Private::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if (handle)
	{
		PRINT_ERR(TXT("Sound file is already open!\n"));
		return false;
	}

	//Create libmpg123 handle
	int error = 0;
	if (!(handle = mpg123_new(NULL, &error)))
	{
		PRINT2_ERR(TXT("Failed to create libmpg123 handle:\n") FMT_chr TXT("\n"), mpg123_plain_strerror(error));
		return false;
	}

	//Initialize the libmpg123 flags
	long flags; double fflags;
	if (mpg123_getparam(handle, MPG123_FLAGS, &flags, &fflags) == MPG123_OK)
	{
		flags |= MPG123_FORCE_FLOAT;
		flags |= MPG123_QUIET;
		if (mpg123_param(handle, MPG123_FLAGS, flags, fflags) != MPG123_OK)
		{
			PRINT2_ERR(TXT("Failed to set up libmpg123 flags: \"") FMT_chr TXT("\"\n"), mpg123_strerror(handle));
			MPG123_DELETE(handle);
			return false;
		}
	}
	else
	{
		PRINT2_ERR(TXT("Failed to query libmpg123 flags: \"") FMT_chr TXT("\"\n"), mpg123_strerror(handle));
		MPG123_DELETE(handle);
		return false;
	}

	//Use pipe?
	const bool bStdIn = (STRCASECMP(fileName, TXT("-")) == 0);

	//Open file in libmpg123
	if ((bStdIn ? mpg123_open_fd(handle, STDIN_FILENO) : MPG123OPEN(handle, fileName)) != MPG123_OK)
	{
		PRINT2_ERR(TXT("Failed to open file for reading: \"") FMT_chr TXT("\"\n"), mpg123_strerror(handle));
		MPG123_DELETE(handle);
		return false;
	}

	//Detect format info
	if (mpg123_getformat(handle, &format.rate, &format.channels, &format.encoding) != MPG123_OK)
	{
		PRINT2_ERR(TXT("Failed to get format info from audio file:\n") FMT_chr TXT("\n"), mpg123_strerror(handle));
		close();
		return false;
	}

	//Ensure that this output format will not change
	if ((mpg123_format_none(handle) != MPG123_OK) || (mpg123_format(handle, format.rate, format.channels, format.encoding) != MPG123_OK))
	{
		PRINT2_ERR(TXT("Failed to set up output format:\n") FMT_chr TXT("\n"), mpg123_strerror(handle));
		close();
		return false;
	}

	//Query output buffer size
	if ((buffSize = mpg123_outblock(handle)) < 1)
	{
		PRINT2_ERR(TXT("Failed to get output buffer size from libmpg123:\n") FMT_chr TXT("\n"), mpg123_strerror(handle));
		close();
		return false;
	}

	//Compute frame size
	frameSize = MPG123_SAMPLESIZE(format.encoding) * format.channels;

	//Allocate temp buffer
	tempBuff = new uint8_t[buffSize];

	return true;
}

bool AudioIO_Mpg123_Private::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format)
{
	return false; /*unsupported*/
}

bool AudioIO_Mpg123_Private::close(void)
{
	bool result = false;

	//Close audio file
	if (handle)
	{
		result = (MPG123CLOSE(handle) == MPG123_OK);
	}

	//Free handle
	MPG123_DELETE(handle);

	//Clear format info
	memset(&format, 0, sizeof(struct mpg123_fmt));

	//Free temp buffer
	buffSize = frameSize = 0;
	MY_DELETE_ARRAY(tempBuff);
	return result;
}

int64_t AudioIO_Mpg123_Private::read(double **buffer, const int64_t count)
{
	if (!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	int64_t offset = 0, remaining = count;
	const int64_t maxRdSize = static_cast<int64_t>(buffSize / frameSize);

	while (remaining > 0)
	{
		//Read data
		const size_t rdBytes = static_cast<size_t>(std::min(remaining, maxRdSize) * frameSize);
		size_t outBytes = 0;
		if (mpg123_read(handle, tempBuff, rdBytes, &outBytes) != MPG123_OK)
		{
			break;
		}

		//Deinterleaving
		const int64_t rdFrames = (int64_t)(outBytes / frameSize);
		switch (format.encoding)
		{
		case MPG123_ENC_FLOAT_32:
			deinterleave(buffer, offset, reinterpret_cast<const float*> (tempBuff), format.channels, rdFrames);
			break;
		case MPG123_ENC_FLOAT_64:
			deinterleave(buffer, offset, reinterpret_cast<const double*>(tempBuff), format.channels, rdFrames);
			break;
		default:
			PRINT2_ERR(TXT("Sample encoding 0x%X is NOT supported!"), format.encoding);
			return 0;
		}

		offset += rdFrames, remaining -= rdFrames;

		if (outBytes < rdBytes)
		{
			break; /*end of file, or read error*/
		}
	}

	//Zero remaining data
	if (remaining > 0)
	{
		for (int c = 0; c < format.channels; c++)
		{
			memset(&buffer[c][offset], 0, sizeof(double) * size_t(remaining));
		}
	}

	return count - remaining;
}

int64_t AudioIO_Mpg123_Private::write(double *const *buffer, const int64_t count)
{
	return -1; /*unsupported*/
}

bool AudioIO_Mpg123_Private::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	if (!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	if (format.channels && format.rate && format.encoding)
	{
		channels = format.channels;
		sampleRate = format.rate;
		if ((length = mpg123_length(handle)) == MPG123_ERR)
		{
			length = INT64_MAX;
		}
		bitDepth = MPG123_SAMPLESIZE(format.encoding) * 8U;
		return true;
	}

	return false;
}

void AudioIO_Mpg123_Private::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	SNPRINTF(buffer, buffSize, TXT("MPG123 Audio"));
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

CHR AudioIO_Mpg123_Private::versionBuffer[256] = { '\0' };
MY_CRITSEC_DECL(AudioIO_Mpg123_Private::mutex_lock);
uint32_t AudioIO_Mpg123_Private::instanceCounter = 0U;

const CHR *AudioIO_Mpg123_Private::libraryVersion(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	if(!versionBuffer[0])
	{
		SNPRINTF(versionBuffer, 256, TXT("libmpg123 API-v%u, written and copyright by Michael Hipp and others."), MPG123_API_VERSION);
	}
	MY_CRITSEC_LEAVE(mutex_lock);
	return versionBuffer;
}

const CHR *const *AudioIO_Mpg123_Private::supportedFormats(const CHR **const list, const uint32_t maxLen)
{
	static const CHR *const MP3_FORMAT = TXT("mp3");
	if (list && (maxLen > 0))
	{
		uint32_t nextPos = 0;
		list[nextPos++] = &MP3_FORMAT[0];
		list[(nextPos < maxLen) ? nextPos : (maxLen - 1)] = NULL;
	}
	return list;
}

bool AudioIO_Mpg123_Private::checkFileType(FILE *const file)
{
	static const uint32_t THRESHOLD = 13U;
	static const size_t BUFF_SIZE = 128U * 1024U;
	uint32_t max_sequence = 0;
	uint8_t *buffer = new uint8_t[BUFF_SIZE];
	size_t count = BUFF_SIZE;
	for (int chunk = 0; (chunk < 64) && (count >= BUFF_SIZE); ++chunk)
	{
		if ((count = fread(buffer, sizeof(uint8_t), BUFF_SIZE, file)) >= MPG123_HDR_SIZE)
		{
			if (checkMpg123Sequence(buffer, count, max_sequence) >= THRESHOLD)
			{
				break; /*early termination*/
			}
		}
	}
	MY_DELETE_ARRAY(buffer);
	return (max_sequence >= THRESHOLD);
}

uint32_t AudioIO_Mpg123_Private::checkMpg123Sequence(const uint8_t *const buffer, const size_t &count, uint32_t &max_sequence)
{
	const size_t limit = count - MPG123_HDR_SIZE;
	uint32_t sequence_len = 0;
	uint8_t prev_idx[3] = { 0, 0, 0 };
	size_t fpos = 0;
	while (fpos < limit)
	{
		if (checkMpg123SyncWord(buffer, fpos))
		{
			const uint8_t versn_idx = ((buffer[fpos + 1U] & ((uint8_t)0x08)) >> 3U);
			const uint8_t layer_idx = ((buffer[fpos + 1U] & ((uint8_t)0x06)) >> 1U);
			const uint8_t bitrt_idx = ((buffer[fpos + 2U] & ((uint8_t)0xF0)) >> 4U);
			const uint8_t srate_idx = ((buffer[fpos + 2U] & ((uint8_t)0x0C)) >> 2U);
			const uint8_t padd_byte = ((buffer[fpos + 2U] & ((uint8_t)0x02)) >> 1U);
			if ((layer_idx > 0x00) && (layer_idx <= 0x03) && (bitrt_idx > 0x00) && (bitrt_idx < 0x0F) && (srate_idx < 0x03))
			{
				const uint32_t bitrt = MPG123_BITRT[versn_idx][layer_idx][bitrt_idx];
				const uint32_t srate = MPG123_SRATE[versn_idx][srate_idx];
				const uint32_t fsize = (MPG123_FSIZE[versn_idx][layer_idx] * bitrt * 1000U / srate) + padd_byte;
				if ((sequence_len < 1U) || ((prev_idx[0] == versn_idx) && (prev_idx[1] == layer_idx) && (prev_idx[2] == srate_idx)))
				{
					prev_idx[0] = versn_idx; prev_idx[1] = layer_idx; prev_idx[2] = srate_idx;
					max_sequence = std::max(max_sequence, ++sequence_len);
					if ((fsize > 0) && ((fpos + fsize) < limit) && checkMpg123SyncWord(buffer, fpos + fsize))
					{
						fpos += fsize;
						continue;
					}
				}
			}
		}
		sequence_len = 0;
		prev_idx[0] = prev_idx[1] = prev_idx[2] = 0;
		fpos++;
	}
	return max_sequence;
}

bool AudioIO_Mpg123_Private::checkMpg123SyncWord(const uint8_t *const buffer, const size_t &fpos)
{
	return (buffer[fpos] == ((uint8_t)0xFF)) && ((buffer[fpos + 1] & ((uint8_t)0xF0)) == ((uint8_t)0xF0));
}

template<typename T>
void AudioIO_Mpg123_Private::deinterleave(double **destination, const int64_t offset, const T *const source, const int &channels, const int64_t &len)
{
	for (int64_t i = 0; i < len; i++)
	{
		for (int c = 0; c < channels; c++)
		{
			destination[c][i + offset] = source[(i * channels) + c];
		}
	}
}

void AudioIO_Mpg123_Private::libraryInit(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	if (!(instanceCounter++))
	{
		if (mpg123_init() != MPG123_OK)
		{
			abort();
		}
	}
	MY_CRITSEC_LEAVE(mutex_lock);
}

void AudioIO_Mpg123_Private::libraryExit(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	if (!(--instanceCounter))
	{
		mpg123_exit();
	}
	MY_CRITSEC_LEAVE(mutex_lock);
}