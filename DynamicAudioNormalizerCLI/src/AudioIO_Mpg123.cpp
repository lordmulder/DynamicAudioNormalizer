//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include <mpg123_msvc.h>
#include <fmt123.h>
#include <stdexcept>
#include <algorithm>

#ifdef __unix
#include <unistd.h>
#else
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#ifdef _WIN32
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#define MPG123OPEN(X,Y) mpg123_topen((X),(Y))
#else
#define MPG123OPEN(X,Y) mpg123_open((X),(Y))
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

private:
	//libmpg123
	mpg123_handle *handle;

	//audio format info
	struct mpg123_fmt format;

	//(De)Interleaving buffer
	uint8_t *tempBuff;
	size_t buffSize;

	//Library info
	static uint32_t instanceCounter;
	static CHR versionBuffer[256];
	static pthread_mutex_t mutex_lock;

	//Helper functions
	static void libraryInit(void);
	static void libraryExit(void);
	static void deinterleave_float32(double **destination, const int64_t offset, const float  *const source, const int &channels, const int64_t &len);
	static void deinterleave_float64(double **destination, const int64_t offset, const double *const source, const int &channels, const int64_t &len);
	static int encodingToBitDepth(const int &encoding);
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

	tempBuff = NULL;
	handle   = NULL;
	buffSize = 0;

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
	const bool bPipe = (STRCASECMP(fileName, TXT("-")) == 0);

	//Open file in libmpg123
	if ((bPipe ? mpg123_open_fd(handle, STDIN_FILENO) : MPG123OPEN(handle, fileName)) != MPG123_OK)
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
		result = (mpg123_close(handle) == MPG123_OK);
	}

	//Free handle
	MPG123_DELETE(handle);

	//Clear format info
	memset(&format, 0, sizeof(struct mpg123_fmt));

	//Free temp buffer
	buffSize = 0;
	MY_DELETE_ARRAY(tempBuff);
	return result;
}

int64_t AudioIO_Mpg123_Private::read(double **buffer, const int64_t count)
{
	if (!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	const size_t frameSize = MPG123_SAMPLESIZE(format.encoding) * format.channels;
	const int64_t maxRdSize = static_cast<int64_t>(buffSize / frameSize);

	int64_t offset = 0;
	int64_t remaining = count;

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
		const int64_t outSamples = (int64_t)(outBytes / frameSize);
		switch (format.encoding)
		{
		case MPG123_ENC_FLOAT_32:
			deinterleave_float32(buffer, offset, reinterpret_cast<float*> (tempBuff), format.channels, outSamples);
			break;
		case MPG123_ENC_FLOAT_64:
			deinterleave_float64(buffer, offset, reinterpret_cast<double*>(tempBuff), format.channels, outSamples);
			break;
		default:
			PRINT2_ERR(TXT("Sample encoding 0x%X is NOT supported!"), format.encoding);
			return 0;
		}

		offset += outSamples;
		remaining -= outSamples;

		if (outBytes < rdBytes)
		{
			PRINT_WRN(TXT("File read error. Read fewer bytes than what was requested!"));
			break;
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
		bitDepth = encodingToBitDepth(format.encoding);
		return true;
	}

	return false;
}

void AudioIO_Mpg123_Private::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	SNPRINTF(buffer, buffSize, TXT("MPEG Audio"));
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

CHR AudioIO_Mpg123_Private::versionBuffer[256] = { '\0' };
pthread_mutex_t AudioIO_Mpg123_Private::mutex_lock = PTHREAD_MUTEX_INITIALIZER;
uint32_t AudioIO_Mpg123_Private::instanceCounter = 0U;

const CHR *AudioIO_Mpg123_Private::libraryVersion(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	if(!versionBuffer[0])
	{
		SNPRINTF(versionBuffer, 256, TXT("libmpg123-v%u, written and copyright by Michael Hipp and others."), MPG123_API_VERSION);
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

void AudioIO_Mpg123_Private::deinterleave_float32(double **destination, const int64_t offset, const float *const source, const int &channels, const int64_t &len)
{
	for (size_t i = 0; i < len; i++)
	{
		for (int c = 0; c < channels; c++)
		{
			destination[c][i + offset] = source[(i * channels) + c];
		}
	}
}

void AudioIO_Mpg123_Private::deinterleave_float64(double **destination, const int64_t offset, const double *const source, const int &channels, const int64_t &len)
{
	for (size_t i = 0; i < len; i++)
	{
		for (int c = 0; c < channels; c++)
		{
			destination[c][i + offset] = source[(i * channels) + c];
		}
	}
}

int AudioIO_Mpg123_Private::encodingToBitDepth(const int &encoding)
{
	switch (encoding)
	{
	case MPG123_ENC_UNSIGNED_8:
	case MPG123_ENC_SIGNED_8:
	case MPG123_ENC_ULAW_8:
	case MPG123_ENC_ALAW_8:
		return 8;
	case MPG123_ENC_SIGNED_16:
	case MPG123_ENC_UNSIGNED_16:
		return 16;
	case MPG123_ENC_SIGNED_24:
	case MPG123_ENC_UNSIGNED_24:
		return 24;
	case MPG123_ENC_SIGNED_32:
	case MPG123_ENC_UNSIGNED_32:
	case MPG123_ENC_FLOAT_32:
		return 32;
	case MPG123_ENC_FLOAT_64:
		return 64;
	default:
		return 16;
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