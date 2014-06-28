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

#include "Common.h"
#include "Parameters.h"
#include "Version.h"

static const CHR *appName(const CHR* argv0)
{
	const CHR *appName  = argv0;
	while(const CHR *temp = STRCHR(appName, TXT('/' ))) appName = &temp[1];
	while(const CHR *temp = STRCHR(appName, TXT('\\'))) appName = &temp[1];
	return appName;
}

int dynamicNormalizerMain(int argc, CHR* argv[])
{
	PRINT(TXT("\nDynamic Audio Normalizer, Version %u.%02u-%u\n"), DYAUNO_VERSION_MAJOR, DYAUNO_VERSION_MINOR, DYAUNO_VERSION_PATCH);
	PRINT(TXT("Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.\n"));
	PRINT(TXT("Built on %s at %s with %s for Win-%s.\n\n"), DYAUNO_BUILD_DATE, DYAUNO_BUILD_TIME, DYAUNO_COMPILER, DYAUNO_ARCH);

	PRINT(TXT("This program is free software: you can redistribute it and/or modify\n"));
	PRINT(TXT("it under the terms of the GNU General Public License <http://www.gnu.org/>.\n"));
	PRINT(TXT("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n\n"));

	Parameters parameters;
	if(!parameters.parseArgs(argc, argv))
	{
		LOG_ERR(TXT("Invalid or incomplete command-line arguments. Type \"%s --help\" for details!"), appName(argv[0]));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int mainEx(int argc, CHR* argv[])
{
	int exitCode = EXIT_SUCCESS;

	try
	{
		exitCode = dynamicNormalizerMain(argc, argv);
	}
	catch(std::exception &e)
	{
		PRINT(TXT("\n\nGURU MEDITATION: Unhandeled C++ exception error: ") FMT_CHAR TXT("\n\n"), e.what());
		exitCode = EXIT_FAILURE;
	}
	catch(...)
	{
		PRINT(TXT("\n\nGURU MEDITATION: Unhandeled unknown C++ exception error!\n\n"));
		exitCode = EXIT_FAILURE;
	}

	return exitCode;
}

int MAIN(int argc, CHR* argv[])
{
	int exitCode = EXIT_SUCCESS;

	if(DYAUNO_DEBUG)
	{
		SYSTEM_INIT();
		exitCode = dynamicNormalizerMain(argc, argv);
	}
	else
	{
		__try
		{
			SYSTEM_INIT();
			exitCode =  mainEx(argc, argv);
		}
		__except(1)
		{
			PRINT(TXT("\n\nGURU MEDITATION: Unhandeled structured exception error!\n\n"));
			exitCode = EXIT_FAILURE;
		}
	}

	return exitCode;
}
