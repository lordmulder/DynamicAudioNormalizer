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

#ifdef _WIN32
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#define SF_OPEN_FILE(X,Y,Z) sf_wchar_open((X),(Y),(Z))
#else
#define SF_OPEN_FILE(X,Y,Z) sf_open((X),(Y),(Z))
#endif

#include "AudioIO_SndFile.h"
#include <Common.h>
#include <Threads.h>

#include <sndfile.h>
#include <stdexcept>
#include <algorithm>

#ifdef __unix
#include <unistd.h>
#else
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#define MY_THROW(X) throw std::runtime_error((X))

///////////////////////////////////////////////////////////////////////////////
// Const
///////////////////////////////////////////////////////////////////////////////

static const struct
{
	const CHR *const extension[3];
	const int format;
	const int sub_format;
	const bool flag[2];
}
g_audio_formats[] =
{
	{ { TXT("wav"),  NULL             }, SF_FORMAT_WAV,  0,                0, 1 },
	{ { TXT("w64"),  NULL             }, SF_FORMAT_W64,  0,                0, 1 },
	{ { TXT("rf64"), NULL             }, SF_FORMAT_RF64, 0,                0, 1 },
	{ { TXT("au"),   NULL             }, SF_FORMAT_AU,   0,                1, 1 },
	{ { TXT("aiff"), NULL             }, SF_FORMAT_AIFF, 0,                1, 1 },
	{ { TXT("ogg"),  TXT("oga"), NULL }, SF_FORMAT_OGG,  SF_FORMAT_VORBIS, 0, 0 },
	{ { TXT("flac"), TXT("fla"), NULL }, SF_FORMAT_FLAC, 0,                1, 0 },
	{ { TXT("raw") , TXT("pcm"), NULL }, SF_FORMAT_RAW,  0,                1, 1 },
	{ { NULL }, 0, 0, 0, 0 }
};

///////////////////////////////////////////////////////////////////////////////
// Private Data
///////////////////////////////////////////////////////////////////////////////

class AudioIO_SndFile_Private
{
public:
	AudioIO_SndFile_Private(void);
	~AudioIO_SndFile_Private(void);

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
	static bool checkFileType(FILE *const file);

private:
	//libsndfile
	SF_INFO info;
	SNDFILE *handle;
	int access;
	bool is_pipe;

	//(De)Interleaving buffer
	double *tempBuff;
	static const size_t BUFF_SIZE = 2048;

	//Library info
	static CHR versionBuffer[256];
	static pthread_mutex_t mutex_lock;

	//Helper functions
	static int formatToBitDepth(const int &format);
	static int formatFromExtension(const CHR *const fileName, const int &bitDepth);
	static int getSubFormat(const int &bitDepth, const bool &eightBitIsSigned, const bool &hightBitdepthSupported);
	static bool checkRawParameters(const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth);
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

AudioIO_SndFile::AudioIO_SndFile(void)
:
	p(new AudioIO_SndFile_Private())
{
}

AudioIO_SndFile::~AudioIO_SndFile(void)
{
	delete p;
}

AudioIO_SndFile_Private::AudioIO_SndFile_Private(void)
{
	access = 0;
	is_pipe = false;
	handle = NULL;
	tempBuff = NULL;
}

AudioIO_SndFile_Private::~AudioIO_SndFile_Private(void)
{
	close();
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_SndFile::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	return p->openRd(fileName, channels, sampleRate, bitDepth);
}

bool AudioIO_SndFile::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format)
{
	return p->openWr(fileName, channels, sampleRate, bitDepth, format);
}

bool AudioIO_SndFile::close(void)
{
	return p->close();
}

int64_t AudioIO_SndFile::read(double **buffer, const int64_t count)
{
	return p->read(buffer, count);
}

int64_t AudioIO_SndFile::write(double *const *buffer, const int64_t count)
{
	return p->write(buffer, count);
}

bool AudioIO_SndFile::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	return p->queryInfo(channels, sampleRate, length, bitDepth);
}

void AudioIO_SndFile::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	p->getFormatInfo(buffer, buffSize);
}

const CHR *AudioIO_SndFile::libraryVersion(void)
{
	return AudioIO_SndFile_Private::libraryVersion();
}

const CHR *const *AudioIO_SndFile::supportedFormats(const CHR **const list, const uint32_t maxLen)
{
	return AudioIO_SndFile_Private::supportedFormats(list, maxLen);
}

bool AudioIO_SndFile::checkFileType(FILE *const file)
{
	return AudioIO_SndFile_Private::checkFileType(file);
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

bool AudioIO_SndFile_Private::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if(handle)
	{
		PRINT_ERR(TXT("Sound file is already open!\n"));
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(SF_INFO));

	//Use pipe?
	const bool bStdIn = (STRCASECMP(fileName, TXT("-")) == 0);
	const bool bPipe = bStdIn && (!FILE_ISREG(STDIN_FILENO));
	
	//Setup info for "raw" input
	if(bPipe)
	{
		if(!checkRawParameters(channels, sampleRate, bitDepth))
		{
			return false;
		}
		info.format = formatFromExtension(TXT("raw"), bitDepth);
		info.channels = channels;
		info.samplerate = sampleRate;
	}

	//Open file in libsndfile
	handle = bStdIn ? sf_open_fd(STDIN_FILENO, SFM_READ, &info, SF_FALSE) : SF_OPEN_FILE(fileName, SFM_READ, &info);

	if(!handle)
	{
		PRINT2_ERR(TXT("Failed to open file for reading: \"") FMT_chr TXT("\"\n"), sf_strerror(NULL));
		return false;
	}

	//Allocate temp buffer
	tempBuff = new double[BUFF_SIZE * info.channels];

	is_pipe = bPipe;
	access = SFM_READ;
	return true;
}

bool AudioIO_SndFile_Private::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format)
{
	if(handle)
	{
		PRINT_ERR(TXT("Sound file is already open!\n"));
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(SF_INFO));

	//Use pipe?
	const bool bStdOut = (STRCASECMP(fileName, TXT("-")) == 0);
	const bool bPipe = bStdOut && (!FILE_ISREG(STDOUT_FILENO));

	//Setup output format
	info.format = format ? formatFromExtension(format, bitDepth) : formatFromExtension(bPipe ? TXT("raw") : fileName, bitDepth);
	info.channels = channels;
	info.samplerate = sampleRate;

	//Open file in libsndfile
	handle = bStdOut ? sf_open_fd(STDOUT_FILENO, SFM_WRITE, &info, SF_FALSE) : SF_OPEN_FILE(fileName, SFM_WRITE, &info);

	if(!handle)
	{
		PRINT2_ERR(TXT("Failed to open file for writing: \"") FMT_chr TXT("\"\n"), sf_strerror(NULL));
		return false;
	}

	//Allocate temp buffer
	tempBuff = new double[BUFF_SIZE * info.channels];

	//Set Vorbis quality
	if((info.format & SF_FORMAT_SUBMASK) == SF_FORMAT_VORBIS)
	{
		double vorbisQualityLevel = 0.6;
		sf_command(handle, SFC_SET_VBR_ENCODING_QUALITY, &vorbisQualityLevel, sizeof(double));
	}

	is_pipe = bPipe;
	access = SFM_WRITE;
	return true;
}

bool AudioIO_SndFile_Private::close(void)
{
	bool result = false;

	//Close sndfile
	if(handle)
	{
		sf_close(handle);
		handle = NULL;
		result = true;
	}

	//Free temp buffer
	MY_DELETE_ARRAY(tempBuff);

	//Clear sndfile status
	access = 0;
	memset(&info, 0, sizeof(SF_INFO));
	
	return result;
}

int64_t AudioIO_SndFile_Private::read(double **buffer, const int64_t count)
{
	if(!handle)
	{
		MY_THROW("Audio file not currently open!");
	}
	if(access != SFM_READ)
	{
		MY_THROW("Audio file not open in READ mode!");
	}

	sf_count_t offset    = 0;
	sf_count_t remaining = count;

	while(remaining > 0)
	{
		//Read data
		const sf_count_t rdSize = std::min(remaining, int64_t(BUFF_SIZE));
		const sf_count_t result = sf_readf_double(handle, tempBuff, rdSize);

		//Deinterleaving
		for(sf_count_t i = 0; i < result; i++)
		{
			for(int c = 0; c < info.channels; c++)
			{
				buffer[c][i + offset] = tempBuff[(i * info.channels) + c];
			}
		}

		offset    += result;
		remaining -= result;

		if(result < rdSize)
		{
			break; /*end of file, or read error*/
		}
	}

	//Zero remaining data
	if(remaining > 0)
	{
		for(int c = 0; c < info.channels; c++)
		{
			memset(&buffer[c][offset], 0, sizeof(double) * size_t(remaining));
		}
	}

	return count - remaining;
}

int64_t AudioIO_SndFile_Private::write(double *const *buffer, const int64_t count)
{
	if(!handle)
	{
		MY_THROW("Audio file not currently open!");
	}
	if(access != SFM_WRITE)
	{
		MY_THROW("Audio file not open in WRITE mode!");
	}

	sf_count_t offset    = 0;
	sf_count_t remaining = count;

	while(remaining > 0)
	{
		const sf_count_t wrSize = std::min(remaining, int64_t(BUFF_SIZE));

		//Interleave data
		for(sf_count_t i = 0; i < wrSize; i++)
		{
			for(int c = 0; c < info.channels; c++)
			{
				tempBuff[(i * info.channels) + c] = buffer[c][i + offset];
			}
		}

		//Write data
		const sf_count_t result = sf_writef_double(handle, tempBuff, wrSize);

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

bool AudioIO_SndFile_Private::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	if(!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	channels = info.channels;
	sampleRate = info.samplerate;
	length = (is_pipe) ? INT64_MAX : info.frames;
	bitDepth = formatToBitDepth(info.format);

	return true;
}

void AudioIO_SndFile_Private::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
	const CHR *format = TXT("Unknown Format");
	const CHR *subfmt = TXT("Unknown SubFmt");

	switch(info.format & SF_FORMAT_TYPEMASK)
	{
		case SF_FORMAT_WAV:    format = TXT("RIFF WAVE"); break;
		case SF_FORMAT_AIFF:   format = TXT("AIFF");      break;
		case SF_FORMAT_AU:     format = TXT("AU/SND");    break;
		case SF_FORMAT_RAW:    format = TXT("Raw Data");  break;
		case SF_FORMAT_W64:    format = TXT("Wave64");    break;
		case SF_FORMAT_WAVEX:  format = TXT("WAVE EX");   break;
		case SF_FORMAT_FLAC:   format = TXT("FLAC");      break;
		case SF_FORMAT_OGG:    format = TXT("Ogg");       break;
		case SF_FORMAT_RF64:   format = TXT("RF64");      break;
	}

	switch(info.format & SF_FORMAT_SUBMASK)
	{
		case SF_FORMAT_PCM_S8: subfmt = TXT("8-Bit Signed");          break;
		case SF_FORMAT_PCM_U8: subfmt = TXT("8-Bit Unsigned");        break;
		case SF_FORMAT_PCM_16: subfmt = TXT("16-Bit Integer");        break;
		case SF_FORMAT_PCM_24: subfmt = TXT("24-Bit Integer");        break;
		case SF_FORMAT_PCM_32: subfmt = TXT("32-Bit Integer");        break;
		case SF_FORMAT_FLOAT:  subfmt = TXT("32-Bit Floating-Point"); break;
		case SF_FORMAT_DOUBLE: subfmt = TXT("64-Bit Floating-Point"); break;
		case SF_FORMAT_VORBIS: subfmt = TXT("Xiph Vorbis");           break;
	}

	SNPRINTF(buffer, buffSize, FMT_CHR TXT(", ") FMT_CHR, format, subfmt);
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

CHR AudioIO_SndFile_Private::versionBuffer[256] = { TXT('\0') };
pthread_mutex_t AudioIO_SndFile_Private::mutex_lock = PTHREAD_MUTEX_INITIALIZER;

const CHR *AudioIO_SndFile_Private::libraryVersion(void)
{
	MY_CRITSEC_ENTER(mutex_lock);
	if(!versionBuffer[0])
	{
		char temp[128] = { '\0' };
		sf_command(NULL, SFC_GET_LIB_VERSION, temp, sizeof(temp));
		SNPRINTF(versionBuffer, 256, FMT_chr TXT(", by Erik de Castro Lopo <erikd@mega-nerd.com>."), temp);
	}
	MY_CRITSEC_LEAVE(mutex_lock);
	return versionBuffer;
}

const CHR *const *AudioIO_SndFile_Private::supportedFormats(const CHR **const list, const uint32_t maxLen)
{
	if (list && (maxLen > 0))
	{
		uint32_t nextPos = 0;
		for (size_t i = 0; g_audio_formats[i].format; ++i)
		{
			if (g_audio_formats[i].extension[0] && (nextPos < maxLen))
			{
				list[nextPos++] = g_audio_formats[i].extension[0];
			}
		}
		list[(nextPos < maxLen) ? nextPos : (maxLen - 1)] = NULL;
	}
	return list;
}

bool AudioIO_SndFile_Private::checkFileType(FILE *const file)
{
	static const size_t BUFF_SIZE = 40;
	uint8_t buffer[BUFF_SIZE];
	const size_t count = fread(buffer, sizeof(uint8_t), BUFF_SIZE, file);
	if (count >= BUFF_SIZE)
	{
		if (((memcmp(&buffer[0], "RIFF", 4) == 0) || (memcmp(&buffer[0], "RF64", 4) == 0)) && (memcmp(&buffer[8], "WAVE", 4) == 0))
		{
			return true;
		}
		if ((memcmp(&buffer[0], "riff\x2E\x91\xCF\x11\xA5\xD6\x28\xDB\x04\xC1\x00\x00", 16) == 0) && (memcmp(&buffer[24], "wave\xF3\xAC\xD3\x11\x8C\xD1\x00\xC0\x4F\x8E\xDB\x8A", 16) == 0))
		{
			return true;
		}
		if ((memcmp(&buffer[0], "FORM", 4) == 0) && (memcmp(&buffer[8], "AIFF", 4) == 0))
		{
			return true;
		}
		if ((memcmp(&buffer[0], "fLaC\0", 5) == 0) || (memcmp(&buffer[0], "OggS\0", 5) == 0) || (memcmp(&buffer[0], ".snd\0", 5) == 0))
		{
			return true;
		}
	}
	return false;
}

int AudioIO_SndFile_Private::formatToBitDepth(const int &format)
{
	switch(format & SF_FORMAT_SUBMASK)
	{
	case SF_FORMAT_PCM_S8:
	case SF_FORMAT_PCM_U8:
		return 8;
	case SF_FORMAT_PCM_16:
		return 16;
	case SF_FORMAT_PCM_24:
		return 24;
	case SF_FORMAT_PCM_32:
	case SF_FORMAT_FLOAT:
		return 32;
	case SF_FORMAT_DOUBLE:
		return 64;
	default:
		return 16;
	}
}

int AudioIO_SndFile_Private::formatFromExtension(const CHR *const fileName, const int &bitDepth)
{
	const CHR *ext = fileName;

	if(const CHR *tmp = STRRCHR(ext, TXT('/' ))) ext = ++tmp;
	if(const CHR *tmp = STRRCHR(ext, TXT('\\'))) ext = ++tmp;
	if(const CHR *tmp = STRRCHR(ext, TXT('.' ))) ext = ++tmp;

	for (size_t i = 0; g_audio_formats[i].format; ++i)
	{
		for (size_t j = 0; g_audio_formats[i].extension[j]; ++j)
		{
			if (STRCASECMP(ext, g_audio_formats[i].extension[j]) == 0)
			{
				return g_audio_formats[i].format | (g_audio_formats[i].sub_format ? g_audio_formats[i].sub_format :
					getSubFormat(bitDepth, g_audio_formats[i].flag[0], g_audio_formats[i].flag[1]));
			}
		}
	}

	return SF_FORMAT_WAV | getSubFormat(bitDepth, 0, 1);
}

int AudioIO_SndFile_Private::getSubFormat(const int &bitDepth, const bool &eightBitIsSigned, const bool &hightBitdepthSupported)
{
	int format = 0;

	switch(bitDepth)
	{
	case 8:
		format = eightBitIsSigned ? SF_FORMAT_PCM_S8 : SF_FORMAT_PCM_U8;
		break;
	case 16:
		format = SF_FORMAT_PCM_16;
		break;
	case 24:
		format = SF_FORMAT_PCM_24;
		break;
	case 32:
		format = hightBitdepthSupported ? SF_FORMAT_FLOAT  : SF_FORMAT_PCM_24;
		break;
	case 64:
		format = hightBitdepthSupported ? SF_FORMAT_DOUBLE : SF_FORMAT_PCM_24;
		break;
	default:
		format = SF_FORMAT_PCM_16;
		break;
	}

	return format;
}

bool AudioIO_SndFile_Private::checkRawParameters(const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
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

	if((bitDepth != 8) && (bitDepth != 16) && (bitDepth != 24) && (bitDepth != 32) && (bitDepth != 64))
	{
		PRINT2_ERR(TXT("Invalid bit depth (%u). Only 8, 16, 24, 32 and 64 bits/sample are supported!\n"), bitDepth);
		return false;
	}

	return true;
}
