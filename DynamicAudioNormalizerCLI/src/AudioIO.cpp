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

#pragma once

#include "AudioIO.h"
#include "AudioIO_SndFile.h"
#include "AudioIO_Mpg123.h"

#include <Common.h>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

//AudioIO libraries
static const uint8_t AUDIO_LIB_NULL       = 0x00u;
static const uint8_t AUDIO_LIB_LIBSNDFILE = 0x01u;
static const uint8_t AUDIO_LIB_LIBMPG123  = 0x02u;

//AudioIO library name to id mappings
static const struct
{
	const CHR name[32];
	const uint8_t id;
}
g_audioIO_mapping[] =
{
	{ TXT("libsndfile"), AUDIO_LIB_LIBSNDFILE },
	{ TXT("libmpg123"),  AUDIO_LIB_LIBMPG123  },
	{ TXT(""),           AUDIO_LIB_NULL       }
};

//Get id from AudioIO library name
static uint8_t parseName(const CHR *const name)
{
	if (name && name[0])
	{
		for (size_t i = 0; g_audioIO_mapping[i].id; ++i)
		{
			if (STRCASECMP(name, g_audioIO_mapping[i].name) == 0)
			{
				return g_audioIO_mapping[i].id;
			}
		}
		return AUDIO_LIB_NULL;
	}
	else
	{
		return AUDIO_LIB_LIBSNDFILE; /*default*/
	}
}

///////////////////////////////////////////////////////////////////////////////
// AudioIO Factory
///////////////////////////////////////////////////////////////////////////////

const CHR *const *AudioIO::getSupportedLibraries(const CHR **const list, const uint32_t maxLen)
{
	if (list && (maxLen > 0))
	{
		uint32_t nextPos = 0;
		for (size_t i = 0; g_audioIO_mapping[i].id; ++i)
		{
			if (g_audioIO_mapping[i].name[0] && (nextPos < maxLen))
			{
				list[nextPos++] = &g_audioIO_mapping[i].name[0];
			}
		}
		list[(nextPos < maxLen) ? nextPos : (maxLen - 1)] = NULL;
	}
	return list;
}

AudioIO *AudioIO::createInstance(const CHR *const name)
{
	switch (parseName(name))
	{
	case AUDIO_LIB_LIBSNDFILE:
		return new AudioIO_SndFile();
	case AUDIO_LIB_LIBMPG123:
		return new AudioIO_Mpg123();
	default:
		throw "Unsupported audio I/O library!";
	}
}

const CHR *const *AudioIO::getSupportedFormats(const CHR **const list, const uint32_t maxLen, const CHR *const name)
{
	switch (parseName(name))
	{
	case AUDIO_LIB_LIBSNDFILE:
		return AudioIO_SndFile::supportedFormats(list, maxLen);
	case AUDIO_LIB_LIBMPG123:
		return AudioIO_Mpg123::supportedFormats(list, maxLen);
	default:
		throw "Unsupported audio I/O library!";
	}
}

const CHR *AudioIO::getLibraryVersion(const CHR *const name)
{
	switch (parseName(name))
	{
	case AUDIO_LIB_LIBSNDFILE:
		return AudioIO_SndFile::libraryVersion();
	case AUDIO_LIB_LIBMPG123:
		return AudioIO_Mpg123::libraryVersion();
	default:
		throw "Unsupported audio I/O library!";
	}
}

///////////////////////////////////////////////////////////////////////////////
// File Type Detection
///////////////////////////////////////////////////////////////////////////////

static const uint32_t SRATE_LUT[2][4] =
{
	{ 44100, 48000, 32000 },
	{ 22050, 24000, 16000 }
};

static const uint32_t BITRT_LUT[2][3][16] =
{
	{
		{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
		{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
		{ 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 }
	},
	{
		{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
		{ 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
		{ 0,  8, 16, 24,  32,  64,  80,  56,  64, 128, 160, 112, 128, 256, 320, 0 }
	},
};

static void detectFromChunk(const uint8_t *const buffer, const size_t &count, uint32_t &max_sequence)
{
	uint32_t prev_srate = 0, sequence_len = 0;
	const size_t limit = count - 2U;
	for (size_t fpos = 0; fpos < limit; ++fpos)
	{
		if ((buffer[fpos] == ((uint8_t)0xFF)) && ((buffer[fpos + 1] & ((uint8_t)0xF0)) == ((uint8_t)0xF0)))
		{
			const uint8_t versn_idx = ((buffer[fpos + 1U] & ((uint8_t)0x08)) >> 3U);
			const uint8_t layer_idx = ((buffer[fpos + 1U] & ((uint8_t)0x06)) >> 1U);
			const uint8_t bitrt_idx = ((buffer[fpos + 2U] & ((uint8_t)0xF0)) >> 4U);
			const uint8_t srate_idx = ((buffer[fpos + 2U] & ((uint8_t)0x0C)) >> 2U);
			const uint8_t padd_byte = ((buffer[fpos + 2U] & ((uint8_t)0x02)) >> 1U);
			if ((layer_idx > 0x00) && (layer_idx <= 0x03) && (bitrt_idx > 0x00) && (bitrt_idx < 0x0F) && (srate_idx < 0x03))
			{
				const uint32_t bitrt = BITRT_LUT[1U - versn_idx][3U - layer_idx][bitrt_idx];
				const uint32_t srate = SRATE_LUT[1U - versn_idx][srate_idx];
				const uint32_t fsize = (144U * bitrt * 1000U / srate) + padd_byte;
				if (fsize > 1)
				{
					if (prev_srate && (prev_srate == srate))
					{
						max_sequence = std::max(max_sequence, ++sequence_len);
					}
					fpos += fsize - 1U;
					prev_srate = srate;
					continue;
				}
			}
		}
		prev_srate = sequence_len = 0;
	}
}

const CHR *AudioIO::detectSourceType(const CHR *const fileName)
{
	uint32_t max_sequence = 0;
	if (fileName && fileName[0] && (STRCASECMP(fileName, TXT("-")) != 0))
	{
		static const size_t BUFF_SIZE = 256 * 1024;
		uint8_t *buffer = new uint8_t[BUFF_SIZE];
		if (FILE *const file = FOPEN(fileName, TXT("rb")))
		{
			size_t count = BUFF_SIZE;
			for (int chunk = 0; (chunk < 32) && (count >= BUFF_SIZE); ++chunk)
			{
				if ((count = fread(buffer, sizeof(uint8_t), BUFF_SIZE, file)) >= 2U)
				{
					detectFromChunk(buffer, count, max_sequence);
				}
			}
			fclose(file);
		}
		MY_DELETE_ARRAY(buffer);
	}
	return (max_sequence >= 16U) ? TXT("libmpg123") : NULL;
}
