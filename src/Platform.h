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

#pragma once

#include <cstdlib>
#include <stdint.h>
#include <cstdio>
#include <cstring>

//============================================================================
// WINDOWS
//============================================================================

#ifdef _WIN32

#include <sys/stat.h>
#include <sys/types.h>

#define TXT(X) L##X
typedef struct _stat64 STAT64;

inline static FILE *FOPEN(const wchar_t *const fileName, const wchar_t *const mode)
{
	FILE *temp;
	if(_wfopen_s(&temp, fileName, mode) == 0)
	{
		return temp;
	}
	return NULL;
}

inline static int FILENO(FILE *const file)
{
	return _fileno(file);
}

inline static int FSTAT64(const int fd, STAT64 *stat)
{
	return _fstat64(fd, stat);
}

inline static int FSEEK64(FILE *const file, int64_t offset, const int origin)
{
	return _fseeki64(file, offset, origin);
}

inline static int64_t FTELL64(FILE *const file)
{
	return _ftelli64(file);
}

inline static int STRCASECMP(const wchar_t *const s1, const wchar_t *const s2)
{
	return _wcsicmp(s1, s2);
}

#endif //_WIN32

//============================================================================
// LINUX
//============================================================================

/*TODO*/
