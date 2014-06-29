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

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////


#define IS_ARG_LONG(ARG_LONG)           (STRCASECMP(argv[pos], TXT("--")TXT(ARG_LONG)) == 0)
#define IS_ARG_SHRT(ARG_SHRT, ARG_LONG) ((STRCASECMP(argv[pos], TXT("-")TXT(ARG_SHRT)) == 0) || (STRCASECMP(argv[pos], TXT("--")TXT(ARG_LONG)) == 0))

#define ENSURE_NEXT_ARG() do \
{ \
	if(++pos >= argc) \
	{ \
		LOG_WRN(TXT("Missing argument for option \"%s\"\n"), argv[pos-1]); \
		return false; \
	} \
} \
while(0)

static bool STR_TO_FLT(double &output, const CHR *str)
{
	double temp;
	if(SSCANF(str, TXT("%lf"), &temp) == 1)
	{
		output = temp;
		return true;
	}
	return false;
}

static bool STR_TO_UINT(uint32_t &output, const CHR *str)
{
	uint32_t temp;
	if(SSCANF(str, TXT("%u"), &temp) == 1)
	{
		output = temp;
		return true;
	}
	return false;
}

#define PARSE_VALUE_FLT(OUT) do \
{ \
	if(!STR_TO_FLT((OUT), argv[pos])) \
	{ \
		LOG_WRN(TXT("Failed to parse floating point value \"%s\"\n"), argv[pos]); \
		return false; \
	} \
} \
while(0)

#define PARSE_VALUE_UINT(OUT) do \
{ \
	if(!STR_TO_UINT((OUT), argv[pos])) \
	{ \
		LOG_WRN(TXT("Failed to parse unsigned integral value \"%s\"\n"), argv[pos]); \
		return false; \
	} \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Parameters Class
///////////////////////////////////////////////////////////////////////////////

Parameters::Parameters(void)
{
	setDefaults();
}

Parameters::~Parameters(void)
{
}

void Parameters::setDefaults(void)
{
	m_sourceFile = NULL;
	m_outputFile = NULL;
	m_dbgLogFile = NULL;

	m_frameLenMsec = 500;
	m_filterSize = 31;
	
	m_channelsCoupled = true;
	m_enableDCCorrection = true;
	
	m_peakValue = 0.95;
	m_maxAmplification = 10.0;
}

bool Parameters::parseArgs(const int argc, CHR* argv[])
{
	int pos = 0;

	while(++pos < argc)
	{
		if(IS_ARG_SHRT("i", "input"))
		{
			ENSURE_NEXT_ARG();
			m_sourceFile = argv[pos];
			continue;
		}
		if(IS_ARG_SHRT("o", "output"))
		{
			ENSURE_NEXT_ARG();
			m_outputFile = argv[pos];
			continue;
		}
		if(IS_ARG_SHRT("l", "logfile"))
		{
			ENSURE_NEXT_ARG();
			m_dbgLogFile = argv[pos];
			continue;
		}
		if(IS_ARG_SHRT("p", "peak"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_FLT(m_peakValue);
			continue;
		}
		if(IS_ARG_SHRT("m", "max-amp"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_FLT(m_maxAmplification);
			continue;
		}
		if(IS_ARG_SHRT("f", "frame-len"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_UINT(m_frameLenMsec);
			continue;
		}
		if(IS_ARG_SHRT("g", "gauss-size"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_UINT(m_filterSize);
			continue;
		}
		if(IS_ARG_LONG("no-coupling"))
		{
			m_channelsCoupled = false;
			continue;
		}
		if(IS_ARG_LONG("no-correct"))
		{
			m_enableDCCorrection = false;
			continue;
		}
		if(IS_ARG_LONG("verbose"))
		{
			setLoggingLevel(2);
			continue;
		}

		LOG_WRN(TXT("Unknown command-line argument \"%s\"\n"), argv[pos]);
		return false;
	}

	if(pos < argc)
	{
		LOG_WRN(TXT("Excess command-line argument \"%s\"\n"), argv[pos]);
		return false;
	}

	return validateParameters();
}

bool Parameters::validateParameters(void)
{
	if(!m_sourceFile)
	{
		LOG_WRN(TXT("Input file not specified!\n"));
		return false;
	}

	if(!m_outputFile)
	{
		LOG_WRN(TXT("Output file not specified!\n"));
		return false;
	}

	if(ACCESS(m_sourceFile, 4) != 0)
	{
		if(errno != ENOENT)
		{
			LOG_WRN(TXT("Selected input file can not be read!\n"));
		}
		else
		{
			LOG_WRN(TXT("Selected input file does not exist!\n"));
		}
		return false;
	}

	if(ACCESS(m_outputFile, 2) != 0)
	{
		if(errno != ENOENT)
		{
			LOG_WRN(TXT("Selected output file is read-only!\n"));
			return false;
		}
	}

	if((m_filterSize < 3) || (m_filterSize > 301))
	{
		LOG_WRN(TXT("Filter size %u is out of range. Must be in the 3 to 301 range!\n"), m_filterSize);
		return false;
	}
	if((m_filterSize % 2) != 1)
	{
		LOG_WRN(TXT("Filter size %u is invalid. Must be an odd value! (i.e. `filter_size mod 2 == 1´)\n"), m_filterSize);
		return false;
	}

	if((m_peakValue < 0.0) || (m_peakValue > 1.0))
	{
		LOG_WRN(TXT("Peak value value %.2f is out of range. Must be in the 0.00 to 1.00 range!\n"), m_peakValue);
		return false;
	}

	if((m_maxAmplification < 1.0) || (m_maxAmplification > 100.0))
	{
		LOG_WRN(TXT("Maximum amplification %.2f is out of range. Must be in the 1.00 to 100.00 range!\n"), m_maxAmplification);
		return false;
	}
	
	if((m_frameLenMsec < 10) || (m_frameLenMsec > 8000))
	{
		LOG_WRN(TXT("Frame length %u is out of range. Must be in the 10 to 8000 range!\n"), m_frameLenMsec);
		return false;
	}

	return true;
}
