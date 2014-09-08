//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

#include "Logging.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

//Stdlib
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

//PThread
#if defined(_WIN32) && defined(_MT)
#define PTW32_STATIC_LIB 1
#endif
#include <pthread.h>

//Globals
static DYNAUDNORM_LOG_CALLBACK *g_loggingCallback = NULL;
static char g_messageBuffer[1024];
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////////
// Helper Macros
///////////////////////////////////////////////////////////////////////////////

#define PTHREAD_LOCK(X) do \
{ \
	if(pthread_mutex_lock(&(X)) != 0) abort(); \
} \
while(0)

#define PTHREAD_UNLOCK(X) do \
{ \
	if(pthread_mutex_unlock(&(X)) != 0) abort(); \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Logging Functions
///////////////////////////////////////////////////////////////////////////////

DYNAUDNORM_LOG_CALLBACK *DYNAUDNORM_LOG_SETCALLBACK(DYNAUDNORM_LOG_CALLBACK *const callback)
{
	PTHREAD_LOCK(g_mutex);

	DYNAUDNORM_LOG_CALLBACK *const oldCallback = g_loggingCallback;
	g_loggingCallback = callback;

	PTHREAD_UNLOCK(g_mutex);
	return oldCallback;
}

void DYNAUDNORM_LOG_POSTMESSAGE(const int &logLevel, const char *const message, ...)
{
	PTHREAD_LOCK(g_mutex);

	if(g_loggingCallback)
	{
		va_list args;
		va_start (args, message);
		vsnprintf(g_messageBuffer, 1024, message, args);
		va_end(args);
		g_loggingCallback(logLevel, g_messageBuffer);
	}

	PTHREAD_UNLOCK(g_mutex);
}
