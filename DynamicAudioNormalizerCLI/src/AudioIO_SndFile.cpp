//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2015 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "AudioIO_SndFile.h"

#include "Common.h"

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
// Private Data
///////////////////////////////////////////////////////////////////////////////

class AudioIO_File_Private
{
public:
	AudioIO_File_Private(void);
	~AudioIO_File_Private(void);

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

	//Virtual I/O
	static sf_count_t vio_get_len(void *user_data);
	static sf_count_t vio_seek(sf_count_t offset, int whence, void *user_data);
	static sf_count_t vio_read(void *ptr, sf_count_t count, void *user_data);
	static sf_count_t vio_write(const void *ptr, sf_count_t count, void *user_data);
	static sf_count_t vio_tell(void *user_data);

	//Library info
	static const char *libraryVersion(void);

private:
	//libsndfile
	FILE *file;
	SF_VIRTUAL_IO vio;
	SF_INFO info;
	SNDFILE *handle;
	int access;

	//(De)Interleaving buffer
	double *tempBuff;
	static const size_t BUFF_SIZE = 2048;

	//Library info
	static char versionBuffer[128];

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
	p(new AudioIO_File_Private())
{
}

AudioIO_SndFile::~AudioIO_SndFile(void)
{
	delete p;
}

AudioIO_File_Private::AudioIO_File_Private(void)
{
	access = 0;
	handle = NULL;
	file = NULL;
	tempBuff = NULL;
}

AudioIO_File_Private::~AudioIO_File_Private(void)
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

bool AudioIO_SndFile::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	return p->openWr(fileName, channels, sampleRate, bitDepth);
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

const char *AudioIO_SndFile::libraryVersion(void)
{
	return AudioIO_File_Private::libraryVersion();
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

#define SETUP_VIO(X) do \
{ \
	(X).get_filelen = AudioIO_File_Private::vio_get_len; \
	(X).read        = AudioIO_File_Private::vio_read;    \
	(X).seek        = AudioIO_File_Private::vio_seek;    \
	(X).tell        = AudioIO_File_Private::vio_tell;    \
	(X).write       = AudioIO_File_Private::vio_write;   \
} \
while(0)

bool AudioIO_File_Private::openRd(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if(handle)
	{
		PRINT_ERR(TXT("Sound file is already open!\n"));
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(SF_INFO));
	memset(&vio,  0, sizeof(SF_VIRTUAL_IO));
	memset(&file, 0, sizeof(FILE*));

	//Use pipe?
	const bool bPipe = (STRCASECMP(fileName, TXT("-")) == 0);

	//Open input file
	if(!bPipe)
	{
		file = FOPEN(fileName, TXT("rb"));
		if(!file)
		{
			return false;
		}
	}

	//Setup the virtual I/O
	SETUP_VIO(vio);
	
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
	handle = bPipe ? sf_open_fd(STDIN_FILENO, SFM_READ, &info, SF_FALSE) : sf_open_virtual(&vio, SFM_READ, &info, file);

	if(!handle)
	{
		if(!bPipe)
		{
			PRINT2_ERR(TXT("Failed to open \"") FMT_CHAR TXT("\"\n"), sf_strerror(NULL));
			fclose(file);
		}
		return false;
	}

	//Allocate temp buffer
	tempBuff = new double[BUFF_SIZE * info.channels];

	access = SFM_READ;
	return true;
}

bool AudioIO_File_Private::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	if(handle)
	{
		PRINT_ERR(TXT("Sound file is already open!\n"));
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(SF_INFO));
	memset(&vio,  0, sizeof(SF_VIRTUAL_IO));
	memset(&file, 0, sizeof(FILE*));

	//Use pipe?
	const bool bPipe = (STRCASECMP(fileName, TXT("-")) == 0);

	//Open file
	if(!bPipe)
	{
		file = FOPEN(fileName, TXT("wb"));
		if(!file)
		{
			return false;
		}
	}
	
	//Setup the virtual I/O
	SETUP_VIO(vio);

	//Setup output format
	info.format = bPipe ? formatFromExtension(TXT("raw"), bitDepth) : formatFromExtension(fileName, bitDepth);
	info.channels = channels;
	info.samplerate = sampleRate;

	//Open file in libsndfile
	handle = bPipe ? sf_open_fd(STDOUT_FILENO, SFM_WRITE, &info, SF_FALSE) : sf_open_virtual(&vio, SFM_WRITE, &info, file);

	if(!handle)
	{
		if(file)
		{
			PRINT2_ERR(TXT("Failed to open \"") FMT_CHAR TXT("\"\n"), sf_strerror(NULL));
			fclose(file);
		}
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

	access = SFM_WRITE;
	return true;
}

bool AudioIO_File_Private::close(void)
{
	bool result = false;

	//Close sndfile
	if(handle)
	{
		sf_close(handle);
		handle = NULL;
		result = true;
	}

	//Close file
	if(file)
	{
		fclose(file);
		file = NULL;
	}

	//Free temp buffer
	MY_DELETE_ARRAY(tempBuff);

	//Clear sndfile status
	access = 0;
	memset(&info, 0, sizeof(SF_INFO));
	memset(&vio, 0, sizeof(SF_VIRTUAL_IO));
	
	return result;
}

int64_t AudioIO_File_Private::read(double **buffer, const int64_t count)
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
			if(file)
			{
				PRINT_WRN(TXT("File read error. Read fewer frames that what was requested!"));
			}
			break;
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

int64_t AudioIO_File_Private::write(double *const *buffer, const int64_t count)
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

bool AudioIO_File_Private::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	if(!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	channels = info.channels;
	sampleRate = info.samplerate;
	length = (file) ? info.frames : INT64_MAX;
	bitDepth = formatToBitDepth(info.format);

	return true;
}

void AudioIO_File_Private::getFormatInfo(CHR *buffer, const uint32_t buffSize)
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

	SNPRINTF(buffer, buffSize, TXT("%s, %s"), format, subfmt);
}

///////////////////////////////////////////////////////////////////////////////
// File I/O Functions
///////////////////////////////////////////////////////////////////////////////

sf_count_t AudioIO_File_Private::vio_get_len(void *user_data)
{
	STAT64 stat;
	if(FSTAT64(FILENO((FILE*)user_data), &stat))
	{
		return -1;
	}
	return stat.st_size;
}

sf_count_t AudioIO_File_Private::vio_seek(sf_count_t offset, int whence, void *user_data)
{
	if(FSEEK64((FILE*)user_data, offset, whence))
	{
		return -1;
	}
	return vio_tell(user_data);
}

sf_count_t AudioIO_File_Private::vio_read(void *ptr, sf_count_t count, void *user_data)
{
	if((count < 0) || (uint64_t(count) > uint64_t(SIZE_MAX)))
	{
		MY_THROW("Read operation requested negative count *or* more than SIZE_MAX bytes!");
	}
	return fread(ptr, 1, size_t(count), (FILE*)user_data);
}

sf_count_t AudioIO_File_Private::vio_write(const void *ptr, sf_count_t count, void *user_data)
{
	if((count < 0) || (uint64_t(count) > uint64_t(SIZE_MAX)))
	{
		MY_THROW("Write operation requested negative count *or* more than SIZE_MAX bytes!");
	}
	return fwrite(ptr, 1, size_t(count), (FILE*)user_data);
}

sf_count_t AudioIO_File_Private::vio_tell(void *user_data)
{
	return FTELL64((FILE*)user_data);
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

char AudioIO_File_Private::versionBuffer[128] = { '\0' };

const char *AudioIO_File_Private::libraryVersion(void)
{
	if(!versionBuffer[0])
	{
		sf_command (NULL, SFC_GET_LIB_VERSION, versionBuffer, sizeof(versionBuffer));
	}
	return versionBuffer;
}

int AudioIO_File_Private::formatToBitDepth(const int &format)
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

int AudioIO_File_Private::formatFromExtension(const CHR *const fileName, const int &bitDepth)
{
	int format = 0;
	const CHR *ext = fileName;

	if(const CHR *tmp = STRRCHR(ext, TXT('/' ))) ext = ++tmp;
	if(const CHR *tmp = STRRCHR(ext, TXT('\\'))) ext = ++tmp;
	if(const CHR *tmp = STRRCHR(ext, TXT('.' ))) ext = ++tmp;

	if(STRCASECMP(ext, TXT("wav")) == 0)
	{
		format = SF_FORMAT_WAV  | getSubFormat(bitDepth, 0, 1);
	}
	else if(STRCASECMP(ext, TXT("w64")) == 0)
	{
		format = SF_FORMAT_W64  | getSubFormat(bitDepth, 0, 1);
	}
	else if(STRCASECMP(ext, TXT("rf64")) == 0)
	{
		format = SF_FORMAT_RF64 | getSubFormat(bitDepth, 0, 1);
	}
	else if(STRCASECMP(ext, TXT("au")) == 0)
	{
		format = SF_FORMAT_AU   | getSubFormat(bitDepth, 1, 1);
	}
	else if(STRCASECMP(ext, TXT("aiff")) == 0)
	{
		format = SF_FORMAT_AIFF | getSubFormat(bitDepth, 1, 1);
	}
	else if((STRCASECMP(ext, TXT("ogg")) == 0) || (STRCASECMP(ext, TXT("oga")) == 0))
	{
		format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
	}
	else if((STRCASECMP(ext, TXT("fla")) == 0) || (STRCASECMP(ext, TXT("flac")) == 0))
	{
		format = SF_FORMAT_FLAC | getSubFormat(bitDepth, 1, 0);
	}
	else if((STRCASECMP(ext, TXT("raw")) == 0) || (STRCASECMP(ext, TXT("pcm")) == 0))
	{
		format = SF_FORMAT_RAW  | getSubFormat(bitDepth, 1, 1);
	}
	else
	{
		format = SF_FORMAT_WAV  | getSubFormat(bitDepth, 0, 1);
	}
	
	return format;
}

int AudioIO_File_Private::getSubFormat(const int &bitDepth, const bool &eightBitIsSigned, const bool &hightBitdepthSupported)
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

bool AudioIO_File_Private::checkRawParameters(const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
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
