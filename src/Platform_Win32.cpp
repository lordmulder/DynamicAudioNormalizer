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

#include "Platform.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <csignal>
#include <io.h>
#include <fcntl.h>

static void my_crash_handler(const char *const message)
{
	__try
	{
		DWORD bytesWritten;
		if(HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE))
		{
			WriteFile(hStdErr, message, lstrlenA(message), &bytesWritten, NULL);
			FlushFileBuffers(hStdErr);
		}
	}
	__except(1)
	{
		/*ignore any exception that might occur in crash handler*/
	}
	for(;;)
	{
		TerminateProcess(GetCurrentProcess(), 666);
	}
}

static void my_invalid_param_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
	my_crash_handler("\n\nGURU MEDITATION: Invalid parameter handler invoked, application will exit!\n\n");
}

static void my_signal_handler(int signal_num)
{
	signal(signal_num, my_signal_handler);
	my_crash_handler("\n\nGURU MEDITATION: Signal handler invoked unexpectedly, application will exit!\n\n");
}

static LONG WINAPI my_exception_handler(struct _EXCEPTION_POINTERS*)
{
	my_crash_handler("\n\nGURU MEDITATION: Unhandeled exception handler invoked, application will exit!\n");
	return EXCEPTION_EXECUTE_HANDLER;
}

void SYSTEM_INIT(void)
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	SetUnhandledExceptionFilter(my_exception_handler);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	_set_invalid_parameter_handler(my_invalid_param_handler);
	
	static const int signal_num[6] = { SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM };

	for(size_t i = 0; i < 6; i++)
	{
		signal(signal_num[i], my_signal_handler);
	}

	_setmode(_fileno(stderr), _O_U8TEXT);
}

#endif //_WIN32
