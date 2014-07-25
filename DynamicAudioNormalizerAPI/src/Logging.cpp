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

#include "Logging.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread 
#endif

#include <cstdlib>
#include <cstdio>
#include <cstdarg>

static DYNAUDNORM_LOG_CALLBACK *g_loggingCallback = NULL;
static THREAD_LOCAL char g_messageBuffer[512];

DYNAUDNORM_LOG_CALLBACK *DYNAUDNORM_LOG_SETCALLBACK(DYNAUDNORM_LOG_CALLBACK *const callback)
{
	DYNAUDNORM_LOG_CALLBACK *const oldCallback = g_loggingCallback;
	g_loggingCallback = callback;
	return oldCallback;
}

void DYNAUDNORM_LOG_POSTMESSAGE(const int &logLevel, const char *const message, ...)
{
	if(g_loggingCallback)
	{
		va_list args;
		va_start (args, message);
		vsnprintf(g_messageBuffer, 512, message, args);
		va_end(args);
		g_loggingCallback(logLevel, message);
	}
}
