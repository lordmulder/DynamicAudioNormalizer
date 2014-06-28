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

#define __STDC_LIMIT_MACROS

#include "AudioFileIO.h"

#include "Common.h"

#include <sndfile.h>

///////////////////////////////////////////////////////////////////////////////
// Private Data
///////////////////////////////////////////////////////////////////////////////


class AudioFileIO_Private
{
public:
	AudioFileIO_Private(void);
	~AudioFileIO_Private(void);

	//Open and Close
	bool openRd(const wchar_t *const fileName);
	bool openWr(const wchar_t *const fileName, const uint32_t channels, const uint32_t sampleRate);
	bool close(void);

	//Read and Write
	int64_t read(double **buffer, const size_t count);
	int64_t write(double *const *buffer, const size_t count);

	//Query info
	bool queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length);

	//Virtual I/O
	static sf_count_t vio_get_len(void *user_data);
	static sf_count_t vio_seek(sf_count_t offset, int whence, void *user_data);
	static sf_count_t vio_read(void *ptr, sf_count_t count, void *user_data);
	static sf_count_t vio_write(const void *ptr, sf_count_t count, void *user_data);
	static sf_count_t vio_tell(void *user_data);

private:
	//libsndfile
	FILE *file;
	SF_VIRTUAL_IO vio;
	SF_INFO info;
	SNDFILE *handle;
	int access;

	//(De)Interleaving
	double *tempBuff;
	size_t tempSize;
};

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

AudioFileIO::AudioFileIO(void)
:
	p(new AudioFileIO_Private())
{
}

AudioFileIO::~AudioFileIO(void)
{
	delete p;
}

AudioFileIO_Private::AudioFileIO_Private(void)
{
	access = 0;
	handle = NULL;
	file = NULL;
	tempBuff = NULL;
	tempSize = 0;
}

AudioFileIO_Private::~AudioFileIO_Private(void)
{
	close();
	MY_DELETE_ARRAY(tempBuff);
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

bool AudioFileIO::openRd(const wchar_t *const fileName)
{
	p->openRd(fileName);
}

bool AudioFileIO::openWr(const wchar_t *const fileName, const uint32_t channels, const uint32_t sampleRate)
{
	p->openWr(fileName, channels, sampleRate);
}

bool AudioFileIO::close(void)
{
	p->close();
}

int64_t AudioFileIO::read(double **buffer, const size_t count)
{
	p->read(buffer, count);
}

int64_t AudioFileIO::write(double *const *buffer, const size_t count)
{
	p->write(buffer, count);
}

bool AudioFileIO::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length)
{
	p->queryInfo(channels, sampleRate, length);
}

///////////////////////////////////////////////////////////////////////////////
// Internal Functions
///////////////////////////////////////////////////////////////////////////////

#define SETUP_VIO(X) do \
{ \
	(X).get_filelen = AudioFileIO_Private::vio_get_len; \
	(X).read        = AudioFileIO_Private::vio_read;    \
	(X).seek        = AudioFileIO_Private::vio_seek;    \
	(X).tell        = AudioFileIO_Private::vio_tell;    \
	(X).write       = AudioFileIO_Private::vio_write;   \
} \
while(0)

bool AudioFileIO_Private::openRd(const wchar_t *const fileName)
{
	if(handle)
	{
		LOG_ERR("AudioFileIO: Sound file is already open!");
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(SF_INFO));
	memset(&vio, 0, sizeof(SF_VIRTUAL_IO));

	//Open file
	if(STRCASECMP(fileName, TXT("-")))
	{
		file = FOPEN(fileName, TXT("rb"));
		if(!file)
		{
			return false;
		}
	}
	else
	{
		file = stdin;
		info.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16;
		info.channels = 2;
		info.samplerate = 44100;
	}

	//Setup the virtual I/O
	SETUP_VIO(vio);
	
	//Open file in libsndfile
	handle = sf_open_virtual(&vio, SFM_READ, &info, file);

	if(!handle)
	{
		LOG_ERR("AudioFileIO: File open error: %s", sf_strerror(NULL));
		fclose(file);
		return false;
	}

	access = SFM_READ;
	return true;
}

bool AudioFileIO_Private::openWr(const wchar_t *const fileName, const uint32_t channels, const uint32_t sampleRate)
{
	if(handle)
	{
		LOG_ERR("AudioFileIO: Sound file is already open!");
		return false;
	}

	//Initialize
	memset(&info, 0, sizeof(SF_INFO));
	memset(&vio, 0, sizeof(SF_VIRTUAL_IO));

	//Open file
	if(STRCASECMP(fileName, TXT("-")))
	{
		file = FOPEN(fileName, TXT("wb"));
		if(!file)
		{
			return false;
		}
	}
	else
	{
		file = stdout;
	}
	
	//Setup the virtual I/O
	SETUP_VIO(vio);

	//Setup output format
	info.format = (STRCASECMP(fileName, TXT("-")) ? SF_FORMAT_WAV : SF_FORMAT_RAW) |SF_FORMAT_PCM_16;
	info.channels = channels;
	info.samplerate = sampleRate;

	//Open file in libsndfile
	handle = sf_open_virtual(&vio, SFM_WRITE, &info, file);

	if(!handle)
	{
		LOG_ERR("AudioFileIO: File open error: %s", sf_strerror(NULL));
		fclose(file);
		return false;
	}

	access = SFM_WRITE;
	return true;
}

bool AudioFileIO_Private::close(void)
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
		if((file != stdin) && (file != stdout))
		{
			fclose(file);
		}
		file = NULL;
	}

	//Clear sndfile status
	access = 0;
	memset(&info, 0, sizeof(SF_INFO));
	memset(&vio, 0, sizeof(SF_VIRTUAL_IO));
	
	return result;
}

int64_t AudioFileIO_Private::read(double **buffer, const size_t count)
{
	if(!handle)
	{
		MY_THROW("Audio file not currently open!");
	}
	if(access != SFM_READ)
	{
		MY_THROW("Audio file not open in READ mode!");
	}

	//Create temp buffer
	if((!tempBuff) || (tempSize < (count * info.channels)))
	{
		MY_DELETE_ARRAY(tempBuff);
		tempBuff = new double[count * info.channels];
	}

	//Read data
	const sf_count_t result = sf_readf_double(handle, tempBuff, count);

	//Deinterleaving
	for(sf_count_t i = 0; i < result; i++)
	{
		for(int c = 0; c < info.channels; c++)
		{
			buffer[c][i] = tempBuff[(i*info.channels)+c];
		}
	}

	//Zero remaining data
	for(sf_count_t i = result; i < sf_count_t(count); i++)
	{
		for(int c = 0; c < info.channels; c++)
		{
			buffer[c][i] = 0.0;
		}
	}

	return result;
}

int64_t AudioFileIO_Private::write(double *const *buffer, const size_t count)
{
	if(!handle)
	{
		MY_THROW("Audio file not currently open!");
	}
	if(access != SFM_WRITE)
	{
		MY_THROW("Audio file not open in WRITE mode!");
	}

	//Create temp buffer
	if((!tempBuff) || (tempSize < (count * info.channels)))
	{
		MY_DELETE_ARRAY(tempBuff);
		tempBuff = new double[count * info.channels];
	}

	//Interleave data
	for(size_t i = 0; i < count; i++)
	{
		for(int c = 0; c < info.channels; c++)
		{
			tempBuff[(i*info.channels)+c] = buffer[c][i];
		}
	}

	//Write data
	return sf_writef_double(handle, tempBuff, count);
}

bool AudioFileIO_Private::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length)
{
	if(!handle)
	{
		MY_THROW("Audio file not currently open!");
	}

	channels = info.channels;
	sampleRate = info.samplerate;
	length = info.frames;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// File I/O Functions
///////////////////////////////////////////////////////////////////////////////

sf_count_t AudioFileIO_Private::vio_get_len(void *user_data)
{
	STAT64 stat;
	if(FSTAT64(FILENO((FILE*)user_data), &stat))
	{
		return -1;
	}
	return stat.st_size;
}

sf_count_t AudioFileIO_Private::vio_seek(sf_count_t offset, int whence, void *user_data)
{
	if(FSEEK64((FILE*)user_data, offset, whence))
	{
		return -1;
	}
	return vio_tell(user_data);
}

sf_count_t AudioFileIO_Private::vio_read(void *ptr, sf_count_t count, void *user_data)
{
	if((count < 0) || (uint64_t(count) > uint64_t(SIZE_MAX)))
	{
		MY_THROW("Read operation requested negative count *or* more than SIZE_MAX bytes!");
	}
	return fread(ptr, 1, size_t(count), (FILE*)user_data);
}

sf_count_t AudioFileIO_Private::vio_write(const void *ptr, sf_count_t count, void *user_data)
{
	if((count < 0) || (uint64_t(count) > uint64_t(SIZE_MAX)))
	{
		MY_THROW("Write operation requested negative count *or* more than SIZE_MAX bytes!");
	}
	return fwrite(ptr, 1, size_t(count), (FILE*)user_data);
}

sf_count_t AudioFileIO_Private::vio_tell(void *user_data)
{
	return FTELL64((FILE*)user_data);
}

