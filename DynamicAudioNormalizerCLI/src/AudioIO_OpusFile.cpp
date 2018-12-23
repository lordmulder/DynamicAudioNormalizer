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
#include <algorithm>

#ifdef __unix
#include <unistd.h>
#else
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#define OPUS_SRATE 48000U
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

	//properties
	int chanCount;
	bool downmix;

	//(De)Interleaving buffer
	float *tempBuff;
	size_t buffSize;

	//Library info
	static MY_CRITSEC_DECL(mutex_lock);
	static CHR versionBuffer[256];

	//Helper functions
	template<typename T> static void deinterleave(double **destination, const int64_t offset, const T *const source, const int &channels, const int64_t &len);
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
	tempBuff  = NULL;
	handle    = NULL;
	stream    = NULL;
	buffSize  = 0;
	chanCount = 0;
	downmix   = false;

	memset(&callbacks, 0, sizeof(OpusFileCallbacks));
}

AudioIO_OpusFile_Private::~AudioIO_OpusFile_Private(void)
{
	close();
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
		stream = NULL;
		memset(&callbacks, 0, sizeof(OpusFileCallbacks));
		return false;
	}

	//Is seekable?
	if (op_seekable(handle))
	{
		//Get the number of links
		const int linkCount = op_link_count(handle);
		if (linkCount < 1)
		{
			MY_THROW("Insanity: Link count is zero or negative!");
		}

		//Get channel count
		chanCount = op_channel_count(handle, 0);
		PRINT(TXT("op_channel_count[%d] = %d\n"), 0, chanCount);
		if (chanCount < 1)
		{
			MY_THROW("Insanity: Channel count is zero or negative!");
		}

		//Too many channels for us?
		downmix = false;
		if (chanCount > 8)
		{
			downmix = true, chanCount = 2; /*enforce stereo*/
		}

		//Check consistency of channel count for *all* links
		if ((!downmix) && (linkCount > 1))
		{
			for (int link = 1; link < linkCount; ++link)
			{
				const int chanCountNext = op_channel_count(handle, link);
				PRINT(TXT("op_channel_count[%d] = %d\n"), link, chanCountNext);
				if (chanCountNext < 1)
				{
					MY_THROW("Insanity: Channel count is zero or negative!");
				}
				if (chanCountNext != chanCount)
				{
					downmix = true, chanCount = 2; /*enforce stereo*/
					break;
				}
			}
		}
	}
	else
	{
		//For non-seekable stream, we can't do anything but enforce stereo!
		PUTS(TXT("Not seekable!\n"));
		downmix = true, chanCount = 2;
	}

	//Allocate I/O buffer
	tempBuff = new float[buffSize = (OPUS_SRATE * chanCount)]; /*1 sec per channel*/

	return true;
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
		result = true;
	}

	//Clear callbacks
	memset(&callbacks, 0, sizeof(OpusFileCallbacks));

	//Free temp buffer
	MY_DELETE_ARRAY(tempBuff);
	buffSize = chanCount = 0, downmix = false, handle = NULL, stream = NULL;
	return result;
}

int64_t AudioIO_OpusFile_Private::read(double **buffer, const int64_t count)
{
	if (!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	int64_t offset = 0, remaining = count;

	//Read next chunk of data
	while (remaining > 0)
	{
		int result, link = -1;
		const int rdSize = static_cast<int>(std::min(remaining * chanCount, static_cast<int64_t>(buffSize)));

		//Read next chunk of data
		for(int retry = 0; retry < 9999; ++retry)
		{
			result = downmix ? op_read_float_stereo(handle, tempBuff, rdSize) : op_read_float(handle, tempBuff, rdSize, &link);
			if (result != OP_HOLE)
			{
				break; /*no more holes*/
			}
		}

		//Check result for errors
		if (result <= 0)
		{
			break; /*end of file, or read error*/
		}

		//Check number of channels
		if ((!downmix) && (op_channel_count(handle, link) != chanCount))
		{
			MY_THROW("Number of channels inconsistent!");
		}

		//Deinterleaving
		deinterleave(buffer, offset, tempBuff, chanCount, result);
		offset += result, remaining -= result;
	}

	//Zero remaining data
	if (remaining > 0)
	{
		for (int c = 0; c < chanCount; c++)
		{
			memset(&buffer[c][offset], 0, sizeof(double) * size_t(remaining));
		}
	}

	return count - remaining;
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

	//Try to compute number of samples
	const ogg_int64_t samplesTotal = op_pcm_total(handle, -1);

	//Set up audio properties
	channels   = (uint32_t)chanCount;
	sampleRate = OPUS_SRATE;
	length     = (samplesTotal > 0) ? samplesTotal : INT64_MAX;
	bitDepth   = 32U;

	return true;
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
	static const size_t BUFF_SIZE = 4096U;
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
