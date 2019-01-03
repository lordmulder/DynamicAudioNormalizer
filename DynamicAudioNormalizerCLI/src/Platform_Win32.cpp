//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2019 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "Platform.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <csignal>
#include <io.h>
#include <fcntl.h>

#ifdef __MINGW32__
extern "C"
{
	typedef struct { int newmode; } _startupinfo;
	int __cdecl __declspec(dllimport) __wgetmainargs (int *_Argc, wchar_t ***_Argv, wchar_t ***_Env, int _DoWildCard, _startupinfo * _StartInfo);
	int wmain(int argc, CHR* argv[]);
}
#endif //__MINGW32__

static void my_crash_handler(const char *const message)
{
	TRY_SEH
	{
		DWORD bytesWritten;
		if(HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE))
		{
			WriteFile(hStdErr, message, lstrlenA(message), &bytesWritten, NULL);
			FlushFileBuffers(hStdErr);
		}
	}
	CATCH_SEH
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

	if(signal_num != SIGINT)
	{
		my_crash_handler("\n\nGURU MEDITATION: Signal handler has been invoked, application will exit!\n\n");
	}
	else
	{
		my_crash_handler("\n\nGURU MEDITATION: Operation has been interrupted, application will exit!\n\n");
	}
}

static LONG WINAPI my_exception_handler(struct _EXCEPTION_POINTERS*)
{
	my_crash_handler("\n\nGURU MEDITATION: Unhandeled exception handler invoked, application will exit!\n");
	return EXCEPTION_EXECUTE_HANDLER;
}

void SYSTEM_INIT(const bool &debugMode)
{
	static const int signal_num[6] = { SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM };

	if(!debugMode)
	{
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
		SetUnhandledExceptionFilter(my_exception_handler);
		_set_invalid_parameter_handler(my_invalid_param_handler);

		for(size_t i = 0; i < 6; i++)
		{
			signal(signal_num[i], my_signal_handler);
		}
	}

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	_setmode(_fileno(stdin ), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
}

//MinGW does *not* provide a proper wmain() entry point, so we have to emulate it here!
#ifdef __MINGW32__
int main()
{
	int argc;
	wchar_t **argv_wide, **envp_wide;
	_startupinfo startup = { 0 };
	if(__wgetmainargs(&argc, &argv_wide, &envp_wide, 0, &startup) != 0)
	{
		abort();
	}
	return wmain(argc, argv_wide);
}
#endif //__MINGW32__

#endif //_WIN32
