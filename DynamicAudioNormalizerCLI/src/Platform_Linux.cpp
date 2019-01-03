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

#ifdef __linux

#pragma GCC diagnostic ignored "-Wunused-result"

#include <csignal>
#include <unistd.h>

static void my_crash_handler(const char *const message)
{
	try
	{
		write(STDERR_FILENO, message, strlen(message));
		fsync(STDERR_FILENO);
	}
	catch(...)
	{
		/*ignore any exception*/
	}
	for(;;)
	{
		_exit(666);
	}
}

static void my_signal_handler(int signal_num)
{
	if(signal_num != SIGINT)
	{
		my_crash_handler("\n\nGURU MEDITATION: Signal handler has been invoked, application will exit!\n\n");
	}
	else
	{
		my_crash_handler("\n\nGURU MEDITATION: Operation has been interrupted, application will exit!\n\n");
	}
}

void SYSTEM_INIT(const bool &debugMode)
{
	static const int signal_num[6] = { SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM };

	if(!debugMode)
	{
		for(size_t i = 0; i < 6; i++)
		{
			struct sigaction sa;
			sa.sa_handler = my_signal_handler;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = SA_RESTART;
			sigaction(signal_num[i], &sa, NULL);
		}
	}
}

#endif //__linux

