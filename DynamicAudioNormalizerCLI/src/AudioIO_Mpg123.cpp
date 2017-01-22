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
#include "Common.h"

#include <mpg123.h>
#include <algorithm>

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

	//(De)Interleaving buffer
	double *tempBuff;
	static const size_t BUFF_SIZE = 2048;

	//Library info
	static CHR versionBuffer[256];

	//Helper functions
	static int formatToBitDepth(const int &format);
	static int formatFromExtension(const CHR *const fileName, const int &bitDepth);
	static int getSubFormat(const int &bitDepth, const bool &eightBitIsSigned, const bool &hightBitdepthSupported);
	static bool checkRawParameters(const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth);
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
}

AudioIO_Mpg123_Private::~AudioIO_Mpg123_Private(void)
{
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
	return false;
}

bool AudioIO_Mpg123_Private::openWr(const CHR *const fileName, const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth, const CHR *const format)
{
	return false;
}

bool AudioIO_Mpg123_Private::close(void)
{
	return false;
}

int64_t AudioIO_Mpg123_Private::read(double **buffer, const int64_t count)
{
	return -1;
}

int64_t AudioIO_Mpg123_Private::write(double *const *buffer, const int64_t count)
{
	return -1;
}

bool AudioIO_Mpg123_Private::queryInfo(uint32_t &channels, uint32_t &sampleRate, int64_t &length, uint32_t &bitDepth)
{
	return false;
}

void AudioIO_Mpg123_Private::getFormatInfo(CHR *buffer, const uint32_t buffSize)
{
}

///////////////////////////////////////////////////////////////////////////////
// Static Functions
///////////////////////////////////////////////////////////////////////////////

CHR AudioIO_Mpg123_Private::versionBuffer[256] = { '\0' };

const CHR *AudioIO_Mpg123_Private::libraryVersion(void)
{
	if(!versionBuffer[0])
	{
		SNPRINTF(versionBuffer, 256, TXT("libmpg123 v%u, written and copyright by Michael Hipp and others."), 0);
	}
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

int AudioIO_Mpg123_Private::formatToBitDepth(const int &format)
{
	return 0;
}

int AudioIO_Mpg123_Private::formatFromExtension(const CHR *const fileName, const int &bitDepth)
{
	return 0;
}

int AudioIO_Mpg123_Private::getSubFormat(const int &bitDepth, const bool &eightBitIsSigned, const bool &hightBitdepthSupported)
{
	return 0;
}

bool AudioIO_Mpg123_Private::checkRawParameters(const uint32_t channels, const uint32_t sampleRate, const uint32_t bitDepth)
{
	return false;
}
