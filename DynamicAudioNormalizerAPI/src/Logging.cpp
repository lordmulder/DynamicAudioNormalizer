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

#include <cstdlib>
#include <cstdio>
#include <cstdarg>

static DYNAUDNORM_LOG_CALLBACK *g_loggingCallback = NULL;

DYNAUDNORM_LOG_CALLBACK *DYNAUDNORM_LOG_SETCALLBACK(DYNAUDNORM_LOG_CALLBACK *const callback)
{
	DYNAUDNORM_LOG_CALLBACK *const oldCallback = g_loggingCallback;
	g_loggingCallback = callback;
	return oldCallback;
}

void DYNAUDNORM_LOG_POSTMESSAGE(const int &logLevel, const char *const message, ...)
{
	static const size_t BUFF_SIZE = 512;
	char messageBuffer[BUFF_SIZE];

	if(g_loggingCallback)
	{
		va_list args;
		va_start (args, message);
		vsnprintf(messageBuffer, BUFF_SIZE, message, args);
		va_end(args);
		g_loggingCallback(logLevel, messageBuffer);
	}
}
