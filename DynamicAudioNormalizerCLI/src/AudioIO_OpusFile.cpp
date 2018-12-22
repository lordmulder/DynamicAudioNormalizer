//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2018 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "AudioIO_OpusFile.h"
#include <Common.h>
#include <Threads.h>
#include <opus.h>
#include <opusfile.h>

//#include <stdexcept>
//#include <algorithm>

#ifdef __unix
#include <unistd.h>
#else
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#define MY_THROW(X) throw std::runtime_error((X))

///////////////////////////////////////////////////////////////////////////////
// Private Data
///////////////////////////////////////////////////////////////////////////////

class AudioIO_OpusFile_Private
{
public:
	AudioIO_OpusFile_Private(void);
	~AudioIO_OpusFile_Private(void);

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
	//opusfile
	OggOpusFile *handle;
	OpusFileCallbacks callbacks;
	void *stream;

	//audio format info
	//struct mpg123_fmt format;

	//(De)Interleaving buffer
	uint8_t *tempBuff;
	size_t frameSize, buffSize;

	//Library info
	static MY_CRITSEC_DECL(mutex_lock);
	static CHR versionBuffer[256];

	//Helper functions
	template<typename T> static void deinterleave(double **destination, const int64_t offset, const T *const source, const int &channels, const int64_t &len);
	static void libraryInit(void);
	static void libraryExit(void);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

AudioIO_OpusFile::AudioIO_OpusFile(void)
:
	p(new AudioIO_OpusFile_Private())
{
}

AudioIO_OpusFile::~AudioIO_OpusFile(void)
{
	delete p;
}

AudioIO_OpusFile_Private::AudioIO_OpusFile_Private(void)
{
	libraryInit();

	tempBuff  = NULL;
	handle    = NULL;
	stream    = NULL;
	frameSize = 0;
	buffSize  = 0;

	memset(&callbacks, 0, sizeof(OpusFileCallbacks));
}

AudioIO_OpusFile_Private::~AudioIO_OpusFile_Private(void)
{
	close();
	libraryExit();
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_OpusFile::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	return p->openRd(fileName, channels, sampleRate, bitDepth);
}

bool AudioIO_OpusFile::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format)
{
	return p->openWr(fileName, channels, sampleRate, bitDepth, format);
}

bool AudioIO_OpusFile::close(void)
{
	return p->close();
}

int64_t AudioIO_OpusFile::read(double **buffer, const int64_t count)
{
	return p->read(buffer, count);
}

int64_t AudioIO_OpusFile::write(double *const *buffer, const int64_t count)
{
	return p->write(buffer, count);
}

bool AudioIO_OpusFile::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	return p->queryInfo(channels, sampleRate, length, bitDepth);
}

void AudioIO_OpusFile::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	p->getFormatInfo(buffer, buffSize);
}

const CHR *AudioIO_OpusFile::libraryVersion(void)
{
	return AudioIO_OpusFile_Private::libraryVersion();
}

const CHR *const *AudioIO_OpusFile::supportedFormats(const CHR **const list, const uint32_t maxLen)
{
	return AudioIO_OpusFile_Private::supportedFormats(list, maxLen);
}

bool AudioIO_OpusFile::checkFileType(FILE *const file)
{
	return AudioIO_OpusFile_Private::checkFileType(file);
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_OpusFile_Private::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if (handle)
	{
		PRINT_ERR(TXT("OggOpus file is already open!\n"));
		return false;
	}

	//Open file descriptor
	const int fd = OPEN(fileName, O_RDONLY);
	if (fd < 0)
	{
		PRINT2_ERR(TXT("Failed to open input file for reading! [error: %d]\n"), errno);
		return false;
	}

	//Create callbacks
	if (!(stream = op_fdopen(&callbacks, fd, "rb")))
	{
		PRINT_ERR(TXT("Failed to initialize OpusFile callbacks!\n"));
		CLOSE(fd);
		return false;
	}

	//Create opusfile handle
	int error;
	if (!(handle = op_open_callbacks(stream, &callbacks, NULL, 0U, &error)))
	{
		PRINT2_ERR(TXT("Failed to open OggOpus file for reading [error: %d]\n"), error);
		callbacks.close(stream);
		return false;
	}

	PRINT_WRN("Unimplemented!");
	return false;
}

bool AudioIO_OpusFile_Private::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format)
{
	return false; /*unsupported*/
}

bool AudioIO_OpusFile_Private::close(void)
{
	bool result = false;

	//Close audio file
	if (handle)
	{
		op_free(handle);
		handle = NULL;
		result = true;
	}

	//Clear callbacks and stream
	memset(&callbacks, 0, sizeof(OpusFileCallbacks));
	stream = NULL;

	//Free temp buffer
	buffSize = frameSize = 0;
	MY_DELETE_ARRAY(tempBuff);
	return result;
}

int64_t AudioIO_OpusFile_Private::read(double **buffer, const int64_t count)
{
	if (!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	return 0;
}

int64_t AudioIO_OpusFile_Private::write(double *const *buffer, const int64_t count)
{
	return -1; /*unsupported*/
}

bool AudioIO_OpusFile_Private::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	if (!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	//if (format.channels && format.rate && format.encoding)
	//{
	//	channels = format.channels;
	//	sampleRate = format.rate;
	//	if ((length = mpg123_length(handle)) == MPG123_ERR)
	//	{
	//		length = INT64_MAX;
	//	}
	//	bitDepth = MPG123_SAMPLESIZE(format.encoding) * 8U;
	//	return true;
	//}

	return false;
}

void AudioIO_OpusFile_Private::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	SNPRINTF(buffer, buffSize, TXT("MPG123 Audio"));
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

CHR AudioIO_OpusFile_Private::versionBuffer[256] = { '\0' };
MY_CRITSEC_DECL(AudioIO_OpusFile_Private::mutex_lock);

const CHR *AudioIO_OpusFile_Private::libraryVersion(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	if(!versionBuffer[0])
	{
		SNPRINTF(versionBuffer, 256, FMT_chr TXT(", created by the Xiph.Org Foundation and contributors."), opus_get_version_string());
	}
	MY_CRITSEC_LEAVE(mutex_lock);
	return versionBuffer;
}

const CHR *const *AudioIO_OpusFile_Private::supportedFormats(const CHR **const list, const uint32_t maxLen)
{
	static const CHR *const OPUS_FORMAT = TXT("opus");
	if (list && (maxLen > 0))
	{
		uint32_t nextPos = 0;
		list[nextPos++] = &OPUS_FORMAT[0];
		list[(nextPos < maxLen) ? nextPos : (maxLen - 1)] = NULL;
	}
	return list;
}

bool AudioIO_OpusFile_Private::checkFileType(FILE *const file)
{
	static const size_t BUFF_SIZE = 1024U;
	uint8_t *buffer = new uint8_t[BUFF_SIZE];
	bool result = false;
	
	const size_t count = fread(buffer, sizeof(uint8_t), BUFF_SIZE, file);
	if (count >= 57U)
	{
		OpusHead head;
		result = (op_test(&head, buffer, count) == 0);
	}

	MY_DELETE_ARRAY(buffer);
	return result;
}

template<typename T>
void AudioIO_OpusFile_Private::deinterleave(double **destination, const int64_t offset, const T *const source, const int &channels, const int64_t &len)
{
	for (int64_t i = 0; i < len; i++)
	{
		for (int c = 0; c < channels; c++)
		{
			destination[c][i + offset] = source[(i * channels) + c];
		}
	}
}

void AudioIO_OpusFile_Private::libraryInit(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	MY_CRITSEC_LEAVE(mutex_lock);
}

void AudioIO_OpusFile_Private::libraryExit(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	MY_CRITSEC_LEAVE(mutex_lock);
}