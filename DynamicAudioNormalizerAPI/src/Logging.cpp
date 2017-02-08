//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

//Internal
#include <Common.h>
#include <Threads.h>

//Stdlib
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

//Globals
static MDynamicAudioNormalizer_LogHandler *g_loggingCallback = NULL;
static char g_messageBuffer[1024];
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

///////////////////////////////////////////////////////////////////////////////
// Logging Functions
///////////////////////////////////////////////////////////////////////////////

MDynamicAudioNormalizer_LogHandler *MDynamicAudioNormalizer_setLogHandler(MDynamicAudioNormalizer_LogHandler *const callback)
{
	MDynamicAudioNormalizer_LogHandler *oldValue;

	MY_CRITSEC_ENTER(g_mutex);
	oldValue = g_loggingCallback;
	g_loggingCallback = callback;
	MY_CRITSEC_LEAVE(g_mutex);

	return oldValue;
}

void MDynamicAudioNormalizer_postLogMessage(const int &logLevel, const char *const message, ...)
{
	MY_CRITSEC_ENTER(g_mutex);

	if(g_loggingCallback)
	{
		va_list args;
		va_start (args, message);
		vsnprintf(g_messageBuffer, 1024, message, args);
		va_end(args);
		g_loggingCallback(logLevel, g_messageBuffer);
	}

	MY_CRITSEC_LEAVE(g_mutex);
}
