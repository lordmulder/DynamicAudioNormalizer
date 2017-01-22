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

//============================================================================
// COMMON
//============================================================================

#ifdef _DYNAUDNORM_PLATFORM_SUPPORT
#undef _DYNAUDNORM_PLATFORM_SUPPORT
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <cstdlib>
#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <sys/types.h>
#include <sys/stat.h>

#define PRINT_NFO(X)       do { PRINT(TXT("INFO: ")    X TXT("\n"));              } while(0)
#define PRINT_WRN(X)       do { PRINT(TXT("WARNING: ") X TXT("\n"));              } while(0)
#define PRINT_ERR(X)       do { PRINT(TXT("ERROR: ")   X TXT("\n"));              } while(0)

#define PRINT2_NFO(X, ...) do { PRINT(TXT("INFO: ")    X TXT("\n"), __VA_ARGS__); } while(0)
#define PRINT2_WRN(X, ...) do { PRINT(TXT("WARNING: ") X TXT("\n"), __VA_ARGS__); } while(0)
#define PRINT2_ERR(X, ...) do { PRINT(TXT("ERROR: ")   X TXT("\n"), __VA_ARGS__); } while(0)

void SYSTEM_INIT(const bool &debugMode);

//============================================================================
// WINDOWS | MSVC
//============================================================================

#if defined(_WIN32) && defined(_MSC_VER) /*Windows MSVC only*/

#ifndef _DYNAUDNORM_PLATFORM_SUPPORT
#define _DYNAUDNORM_PLATFORM_SUPPORT 1
#else
#error Whoops, more than one platform has been detected!
#endif

#include <io.h>

#define _TXT(X) L##X
#define TXT(X) _TXT(X)
#define OS_TYPE TXT("Win")
#define FMT_CHR TXT("%s")
#define FMT_chr TXT("%S")
#define MAIN wmain
#define TRY_SEH __try
#define CATCH_SEH __except(1)

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

typedef wchar_t CHR;
typedef struct _stat64 STAT64;

inline static int PRINT(const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = vfwprintf_s(stderr, format, ap);
	va_end(ap);
	return result;
}

inline static void FLUSH(void)
{
	fflush(stderr);
}

inline static FILE *FOPEN(const CHR *const fileName, const CHR *const mode)
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

inline static int ACCESS(const CHR *const fileName, const int mode)
{
	return _waccess(fileName, mode);
}

inline static int STRCASECMP(const CHR *const s1, const CHR *const s2)
{
	return _wcsicmp(s1, s2);
}

inline static const CHR *STRCHR(const CHR *const str, const CHR c)
{
	return wcschr(str, c);
}

inline static const CHR *STRRCHR(const CHR *const str, const CHR c)
{
	return wcsrchr(str, c);
}

inline static int SNPRINTF(CHR *const buffer, size_t buffSize, const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = _vsnwprintf_s(buffer, buffSize, _TRUNCATE, format, ap);
	va_end(ap);
	return result;
}

inline static int SSCANF(const CHR *const str, const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = vswscanf(str, format, ap);
	va_end(ap);
	return result;
}

inline static const CHR *STRNCAT(CHR *const buff, const CHR *const str, const uint32_t len)
{
	wcsncat_s(buff, len, str, _TRUNCATE);
	return buff;
}

#endif //_WIN32

//============================================================================
// WINDOWS | MINGW
//============================================================================

#if defined(_WIN32) && defined(__MINGW32__) /*Windows MinGW only*/

#ifndef _DYNAUDNORM_PLATFORM_SUPPORT
#define _DYNAUDNORM_PLATFORM_SUPPORT 1
#else
#error Whoops, more than one platform has been detected!
#endif

#include <io.h>

#define _TXT(X) L##X
#define TXT(X) _TXT(X)
#define OS_TYPE TXT("Win")
#define FMT_CHR TXT("%ls")
#define FMT_chr TXT("%s")
#define MAIN wmain
#define TRY_SEH if(1)
#define CATCH_SEH else

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

typedef wchar_t CHR;
typedef struct _stati64 STAT64;

inline static int PRINT(const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = vfwprintf(stderr, format, ap);
	va_end(ap);
	return result;
}

inline static void FLUSH(void)
{
	fflush(stderr);
}

inline static FILE *FOPEN(const CHR *const fileName, const CHR *const mode)
{
	return _wfopen(fileName, mode);
}

inline static int FILENO(FILE *const file)
{
	return _fileno(file);
}

inline static int FSTAT64(const int fd, STAT64 *stat)
{
	return _fstati64(fd, stat);
}

inline static int FSEEK64(FILE *const file, int64_t offset, const int origin)
{
	return fseeko64(file, offset, origin); //__mingw_fseeko64(file, offset, origin)
}

inline static int64_t FTELL64(FILE *const file)
{
	return ftello64(file);
}

inline static int ACCESS(const CHR *const fileName, const int mode)
{
	return _waccess(fileName, mode);
}

inline static int STRCASECMP(const CHR *const s1, const CHR *const s2)
{
	return _wcsicmp(s1, s2);
}

inline static const CHR *STRCHR(const CHR *const str, const CHR c)
{
	return wcschr(str, c);
}

inline static const CHR *STRRCHR(const CHR *const str, const CHR c)
{
	return wcsrchr(str, c);
}

inline static int SNPRINTF(CHR *const buffer, size_t buffSize, const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = _vsnwprintf(buffer, buffSize, format, ap);
	buffer[buffSize-1] = 0x00;
	va_end(ap);
	return result;
}

inline static int SSCANF(const CHR *const str, const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = vswscanf(str, format, ap);
	va_end(ap);
	return result;
}

inline static const CHR *STRNCAT(CHR *const buff, const CHR *const str, const uint32_t len)
{
	const size_t offset = wcslen(buffer);
	if (len >= 1)
	{
		return wcsncat(buff, str, len - offset - 1);
	}
	return buff;
}

#endif //_WIN32

//============================================================================
// LINUX
//============================================================================

#ifdef __linux /*Linux only*/

#ifndef _DYNAUDNORM_PLATFORM_SUPPORT
#define _DYNAUDNORM_PLATFORM_SUPPORT 1
#else
#error Whoops, more than one platform has been detected!
#endif

#include <unistd.h>

#define _TXT(X) X
#define TXT(X) _TXT(X)
#define OS_TYPE TXT("Linux")
#define FMT_CHR TXT("%s")
#define FMT_chr TXT("%s")
#define MAIN main
#define TRY_SEH if(1)
#define CATCH_SEH else

typedef char CHR;
typedef struct stat64 STAT64;

inline static int PRINT(const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = vfprintf(stderr, format, ap);
	va_end(ap);
	return result;
}

inline static void FLUSH(void)
{
	fflush(stderr);
}

inline static FILE *FOPEN(const CHR *const fileName, const CHR *const mode)
{
	return fopen(fileName, mode);
}

inline static int FILENO(FILE *const file)
{
	return fileno(file);
}

inline static int FSTAT64(const int fd, STAT64 *stat)
{
	return fstat64(fd, stat);
}

inline static int FSEEK64(FILE *const file, int64_t offset, const int origin)
{
	return fseeko64(file, offset, origin);
}

inline static int64_t FTELL64(FILE *const file)
{
	return ftello64(file);
}

inline static int ACCESS(const CHR *const fileName, const int mode)
{
	return access(fileName, mode);
}

inline static int STRCASECMP(const CHR *const s1, const CHR *const s2)
{
	return strcasecmp(s1, s2);
}

inline static const CHR *STRCHR(const CHR *const str, const CHR c)
{
	return strchr(str, c);
}

inline static const CHR *STRRCHR(const CHR *const str, const CHR c)
{
	return strrchr(str, c);
}

inline static int SNPRINTF(CHR *const buffer, size_t buffSize, const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = vsnprintf(buffer, buffSize, format, ap);
	va_end(ap);
	return result;
}

inline static int SSCANF(const CHR *const str, const CHR *const format, ...)
{
	va_list ap;
	va_start(ap, format);
	const int result = vsscanf(str, format, ap);
	va_end(ap);
	return result;
}

inline static const CHR *STRNCAT(CHR *const buff, const CHR *const str, const uint32_t len)
{
	const size_t offset = strlen(buffer);
	if (len >= 1)
	{
		return strncat(buff, str, len - offset - 1);
	}
	return buff;
}

#endif //__linux

//============================================================================
// VALIDATION
//============================================================================

/*make sure one platform has been initialized!*/
#if !(defined(_DYNAUDNORM_PLATFORM_SUPPORT) && _DYNAUDNORM_PLATFORM_SUPPORT)
#error This platform is *not* currently supported!
#endif
