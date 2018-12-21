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

#include "AudioIO.h"
#include "AudioIO_SndFile.h"
#include "AudioIO_Mpg123.h"

#include <Common.h>
#include <algorithm>

#ifdef __unix
#include <unistd.h>
#else
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

///////////////////////////////////////////////////////////////////////////////
// Helper Functions
///////////////////////////////////////////////////////////////////////////////

//AudioIO libraries
static const uint8_t AUDIO_LIB_NULL       = 0x00u;
static const uint8_t AUDIO_LIB_LIBSNDFILE = 0x01u;
static const uint8_t AUDIO_LIB_LIBMPG123  = 0x02u;

//Function prototypes
typedef const CHR *       (libraryVersion_t)  (void);
typedef const CHR *const *(supportedFormats_t)(const CHR **const list, const uint32_t maxLen);
typedef bool              (checkFileType_t)   (FILE *const);
typedef AudioIO*          (createInstance_t)  (void);

//AudioIO library mappings
static const struct
{
	const CHR                 name[16];
	const uint8_t             id;
	libraryVersion_t   *const libraryVersion;
	supportedFormats_t *const supportedFormats;
	checkFileType_t    *const checkFileType;
	createInstance_t   *const createInstance;
}
g_audioIO_mapping[] =
{
	{ TXT("libsndfile"), AUDIO_LIB_LIBSNDFILE, AudioIO_SndFile::libraryVersion, AudioIO_SndFile::supportedFormats, AudioIO_SndFile::checkFileType, AudioIO_SndFile::createInstance },
	{ TXT("libmpg123"),  AUDIO_LIB_LIBMPG123,  AudioIO_Mpg123 ::libraryVersion, AudioIO_Mpg123 ::supportedFormats, AudioIO_Mpg123 ::checkFileType, AudioIO_Mpg123 ::createInstance },
	{ TXT(""),           AUDIO_LIB_NULL,       NULL,                            NULL,                              NULL,                           NULL                            }
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

//Get id from AudioIO library name
static const CHR* getLibName(const uint8_t id)
{
	for (size_t i = 0; g_audioIO_mapping[i].id; ++i)
	{
		if (g_audioIO_mapping[i].id == id)
		{
			return g_audioIO_mapping[i].name;
		}
	}
	return NULL;
}

//Check file type
static bool checkFileType(bool(*const check)(FILE *const), FILE *const file)
{
	const bool check_result = check(file);
	rewind(file);
	return check_result;
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
	const uint8_t id = parseName(name);
	for (size_t i = 0; g_audioIO_mapping[i].id; ++i)
	{
		if (g_audioIO_mapping[i].id == id)
		{
			return g_audioIO_mapping[i].createInstance();
		}
	}
	throw "Unsupported audio I/O library!";
}

const CHR *const *AudioIO::getSupportedFormats(const CHR **const list, const uint32_t maxLen, const CHR *const name)
{
	const uint8_t id = parseName(name);
	for (size_t i = 0; g_audioIO_mapping[i].id; ++i)
	{
		if (g_audioIO_mapping[i].id == id)
		{
			return g_audioIO_mapping[i].supportedFormats(list, maxLen);
		}
	}
	throw "Unsupported audio I/O library!";
}

const CHR *AudioIO::getLibraryVersion(const CHR *const name)
{
	const uint8_t id = parseName(name);
	for (size_t i = 0; g_audioIO_mapping[i].id; ++i)
	{
		if (g_audioIO_mapping[i].id == id)
		{
			return g_audioIO_mapping[i].libraryVersion();
		}
	}
	throw "Unsupported audio I/O library!";
}

const CHR *AudioIO::detectSourceType(const CHR *const fileName)
{
	uint8_t type = AUDIO_LIB_NULL;
	const bool bStdIn = (STRCASECMP(fileName, TXT("-")) == 0);
	if (bStdIn || PATH_ISREG(fileName))
	{
		if (FILE *const file = bStdIn ? stdin : FOPEN(fileName, TXT("rb")))
		{
			if (FD_ISREG(FILENO(file)))
			{
				for (size_t i = 0; g_audioIO_mapping[i].id; ++i)
				{
					if (checkFileType(g_audioIO_mapping[i].checkFileType, file))
					{
						type = g_audioIO_mapping[i].id;
						break;
					}
				}
			}
			if (!bStdIn)
			{
				fclose(file);
			}
		}
	}
	return getLibName(type);
}
