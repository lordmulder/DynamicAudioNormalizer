//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// http://www.gnu.org/licenses/gpl-3.0.txt
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//===================================================================
// WinMain entry point
//===================================================================

static LONG WINAPI my_exception_handler(struct _EXCEPTION_POINTERS*)
{
	FatalAppExitW(0, L"GURU MEDITATION: Unhandeled exception handler invoked, application will exit!\n");
	TerminateProcess(GetCurrentProcess(), 666);
	return EXCEPTION_EXECUTE_HANDLER;
}

extern "C"
{
	int mainCRTStartup(void);

	int win32EntryPoint(void)
	{
#ifndef _DEBUG
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
		SetUnhandledExceptionFilter(my_exception_handler);
#endif
		return mainCRTStartup();
	}
}

#endif //_WIN32
