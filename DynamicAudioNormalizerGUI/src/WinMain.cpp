//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2015 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

int main(int argc, char* argv[]);

//===================================================================
// WinMain entry point
//===================================================================

#ifdef _DEBUG
#define RELEASE_BUILD 0
#else
#define RELEASE_BUILD 1
#endif

extern "C"
{
	int mainCRTStartup(void);

	int win32EntryPoint(void)
	{
		if(RELEASE_BUILD)
		{
			BOOL debuggerPresent = TRUE;
			if(!CheckRemoteDebuggerPresent(GetCurrentProcess(), &debuggerPresent))
			{
				debuggerPresent = FALSE;
			}
			if(debuggerPresent || IsDebuggerPresent())
			{
				MessageBoxW(NULL, L"Not a debug build. Unload debugger and try again!", L"Debugger", MB_TOPMOST | MB_ICONSTOP);
				return -1;
			}
			else
			{
				return mainCRTStartup();
			}
		}
		else
		{
			return mainCRTStartup();
		}
	}
}

#endif //_WIN32
