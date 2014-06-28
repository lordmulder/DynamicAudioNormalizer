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

#include "Parameters.h"

#include <cerrno>

#define IS_ARG_LONG(ARG_LONG)           (STRCASECMP(argv[pos], TXT("--")TXT(ARG_LONG)) == 0)
#define IS_ARG_SHRT(ARG_SHRT, ARG_LONG) ((STRCASECMP(argv[pos], TXT("-")TXT(ARG_SHRT)) == 0) || (STRCASECMP(argv[pos], TXT("--")TXT(ARG_LONG)) == 0))

Parameters::Parameters(void)
{
	setDefaults();
}

Parameters::~Parameters(void)
{
}
	
bool Parameters::parseArgs(const int argc, CHR* argv[])
{
	int pos = 0;
	const int maxArg = (argc > 0) ? (argc - 1) : 0;

	while(++pos < maxArg)
	{
		if(IS_ARG_SHRT("i", "input"))
		{
			m_sourceFile = argv[++pos];
			continue;
		}
		if(IS_ARG_SHRT("o", "output"))
		{
			m_outputFile = argv[++pos];
			continue;
		}

		LOG_WRN(TXT("Unknown command-line argument: %s"), argv[pos]);
		return false;
	}

	if(pos < argc)
	{
		LOG_WRN(TXT("Excess command-line argument: %s"), argv[pos]);
		return false;
	}

	return validateParameters();
}

void Parameters::setDefaults(void)
{
	m_sourceFile = NULL;
	m_outputFile = NULL;
}

bool Parameters::validateParameters(void)
{
	if(!m_sourceFile)
	{
		LOG_WRN(TXT("Input file not specified!"));
		return false;
	}

	if(!m_outputFile)
	{
		LOG_WRN(TXT("Output file not specified!"));
		return false;
	}

	if(ACCESS(m_sourceFile, 4) != 0)
	{
		if(errno != ENOENT)
		{
			LOG_WRN(TXT("Selected input file can not be read!"));
		}
		else
		{
			LOG_WRN(TXT("Selected input file does not exist!"));
		}
		return false;
	}

	if(ACCESS(m_outputFile, 2) != 0)
	{
		if(errno != ENOENT)
		{
			LOG_WRN(TXT("Selected output file is read-only!"));
			return false;
		}
	}

	return true;
}
