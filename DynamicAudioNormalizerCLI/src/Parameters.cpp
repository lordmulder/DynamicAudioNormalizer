//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2018 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "Parameters.h"
#include "AudioIO.h"

#include <cerrno>

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

#define IS_ARG_LONG(ARG_LONG)           (STRCASECMP(argv[pos], TXT("--") TXT(ARG_LONG)) == 0)
#define IS_ARG_SHRT(ARG_SHRT, ARG_LONG) ((STRCASECMP(argv[pos], TXT("-") TXT(ARG_SHRT)) == 0) || (STRCASECMP(argv[pos], TXT("--") TXT(ARG_LONG)) == 0))

#define ENSURE_NEXT_ARG() do \
{ \
	if(++pos >= argc) \
	{ \
		PRINT2_WRN(TXT("Missing argument for option \"") FMT_CHR TXT("\".\n"), argv[pos-1]); \
		return false; \
	} \
} \
while(0)

#define ENSURE_NEXT_ARGS(N) do \
{ \
	if(((pos++) + (N)) >= argc) \
	{ \
		PRINT2_WRN(TXT("Missing argument(s) for option \"") FMT_CHR TXT("\".\n"), argv[pos-1]); \
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
		PRINT2_WRN(TXT("Failed to parse floating point value \"") FMT_CHR TXT("\"\n"), argv[pos]); \
		return false; \
	} \
} \
while(0)

#define PARSE_VALUE_UINT(OUT) do \
{ \
	if(!STR_TO_UINT((OUT), argv[pos])) \
	{ \
		PRINT2_WRN(TXT("Failed to parse unsigned integral value \"") FMT_CHR TXT("\"\n"), argv[pos]); \
		return false; \
	} \
} \
while(0)

static bool CHECK_ENUM(const CHR *const str, const CHR *const *const values)
{
	for (size_t i = 0; values[i]; ++i)
	{
		if (STRCASECMP(str, values[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

static const CHR *JOIN_STR(CHR *const buffer, uint32_t buffSize, const CHR *const *const values, const CHR *const separator)
{
	if (buffer && (buffSize > 0))
	{
		buffer[0] = TXT('\0');
		for (size_t i = 0; values[i]; ++i)
		{
			if (i > 0)
			{
				STRNCAT(buffer, separator, buffSize);
			}
			STRNCAT(buffer, values[i], buffSize);
		}
	}
	return buffer;
}

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
	m_sourceFile    = NULL;
	m_sourceLibrary = NULL;
	m_outputFile    = NULL;
	m_outputFormat  = NULL;
	m_dbgLogFile    = NULL;

	m_frameLenMsec = 500;
	m_filterSize   = 31;
	
	m_inputChannels   = 0;
	m_inputSampleRate = 0;
	m_inputBitDepth   = 0;

	m_channelsCoupled    = true;
	m_enableDCCorrection = false;
	m_altBoundaryMode    = false;
	m_verboseMode        = false;
	m_showHelp           = false;
	
	m_peakValue        =  0.95;
	m_maxAmplification = 10.00;
	m_targetRms        =  0.00;
	m_compressFactor   =  0.00;
}

bool Parameters::parseArgs(const int argc, CHR* argv[])
{
	int pos = 0;

	while(++pos < argc)
	{
		/*help screen*/
		if(IS_ARG_SHRT("h", "help"))
		{
			m_showHelp = true;
			break;
		}

		/*input and output*/
		if(IS_ARG_SHRT("i", "input"))
		{
			ENSURE_NEXT_ARG();
			m_sourceFile = argv[pos];
			continue;
		}
		if (IS_ARG_SHRT("d", "input-lib"))
		{
			ENSURE_NEXT_ARG();
			m_sourceLibrary = argv[pos];
			continue;
		}
		if(IS_ARG_SHRT("o", "output"))
		{
			ENSURE_NEXT_ARG();
			m_outputFile = argv[pos];
			continue;
		}
		if (IS_ARG_SHRT("t", "output-fmt"))
		{
			ENSURE_NEXT_ARG();
			m_outputFormat = argv[pos];
			continue;
		}

		/*algorithm twekas*/
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
		if(IS_ARG_SHRT("p", "peak"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_FLT(m_peakValue);
			continue;
		}
		if(IS_ARG_SHRT("m", "max-gain"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_FLT(m_maxAmplification);
			continue;
		}
		if(IS_ARG_SHRT("r", "target-rms"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_FLT(m_targetRms);
			continue;
		}
		if(IS_ARG_SHRT("s", "compress"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_FLT(m_compressFactor);
			continue;
		}
		if(IS_ARG_SHRT("n", "no-coupling"))
		{
			m_channelsCoupled = false;
			continue;
		}
		if(IS_ARG_SHRT("c", "correct-dc"))
		{
			m_enableDCCorrection = true;
			continue;
		}
		if(IS_ARG_SHRT("b", "alt-boundary"))
		{
			m_altBoundaryMode = true;
			continue;
		}

		/*diagnostics*/
		if(IS_ARG_SHRT("l", "log-file"))
		{
			ENSURE_NEXT_ARG();
			m_dbgLogFile = argv[pos];
			continue;
		}
		if(IS_ARG_SHRT("v", "verbose"))
		{
			m_verboseMode = true;
			continue;
		}

		/*raw input options*/
		if(IS_ARG_LONG("input-chan"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_UINT(m_inputChannels);
			continue;
		}
		if(IS_ARG_LONG("input-rate"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_UINT(m_inputSampleRate);
			continue;
		}
		if(IS_ARG_LONG("input-bits"))
		{
			ENSURE_NEXT_ARG();
			PARSE_VALUE_UINT(m_inputBitDepth);
			continue;
		}

		PRINT2_WRN(TXT("Unknown command-line argument \"") FMT_CHR TXT("\"\n"), argv[pos]);
		return false;
	}

	if((!m_showHelp) && (pos < argc))
	{
		PRINT2_WRN(TXT("Excess command-line argument \"") FMT_CHR TXT("\"\n"), argv[pos]);
		return false;
	}

	return m_showHelp || validateParameters();
}

bool Parameters::validateParameters(void)
{
	if(!m_sourceFile)
	{
		PRINT_WRN(TXT("Input file not specified!\n"));
		return false;
	}
	if(!m_outputFile)
	{
		PRINT_WRN(TXT("Output file not specified!\n"));
		return false;
	}

	/*validate input library*/
	if (m_sourceLibrary)
	{
		const CHR *supportedLibraries[8];
		if (!CHECK_ENUM(m_sourceLibrary, AudioIO::getSupportedLibraries(supportedLibraries, 8)))
		{
			CHR allLibraries[128];
			PRINT2_WRN(TXT("Unknown input library \"") FMT_CHR TXT("\" specified!\nSupported libraries include: ") FMT_CHR TXT("\n"), m_sourceLibrary, JOIN_STR(allLibraries, 128, supportedLibraries, TXT(", ")));
			return false;
		}
	}

	/*validate output format*/
	if(m_outputFormat)
	{
		const CHR *supportedFormats[32];
		if (!CHECK_ENUM(m_outputFormat, AudioIO::getSupportedFormats(supportedFormats, 32)))
		{
			CHR allFormats[256];
			PRINT2_WRN(TXT("Unknown output format \"") FMT_CHR TXT("\" specified!\nSupported formats include: ") FMT_CHR TXT("\n"), m_outputFormat, JOIN_STR(allFormats, 256, supportedFormats, TXT(", ")));
			return false;
		}
	}

	/*check input file*/
	if(STRCASECMP(m_sourceFile, TXT("-")) != 0)
	{
		if(ACCESS(m_sourceFile, R_OK) != 0)
		{
			if(errno != ENOENT)
			{
				PRINT_WRN(TXT("Selected input file can not be read!\n"));
			}
			else
			{
				PRINT_WRN(TXT("Selected input file does not exist!\n"));
			}
			return false;
		}
	}

	/*check output file*/
	if(STRCASECMP(m_outputFile, TXT("-")) != 0)
	{
		if(ACCESS(m_outputFile, W_OK) != 0)
		{
			if(errno != ENOENT)
			{
				PRINT_WRN(TXT("Selected output file is read-only!\n"));
				return false;
			}
		}
	}

	/*check input properties for raw data*/
	if((STRCASECMP(m_sourceFile, TXT("-")) == 0) && (!FD_ISREG(FILENO(stdin))))
	{
		if((m_inputChannels == 0) || (m_inputSampleRate == 0) || (m_inputBitDepth == 0))
		{
			PRINT2_WRN(TXT("Channel-count, sample-rate and bit-depth *must* be specified for pipe'd input!\n"), m_filterSize);
			return false;
		}
	}
	else
	{
		if((m_inputChannels != 0) || (m_inputSampleRate != 0) || (m_inputBitDepth != 0))
		{
			PRINT2_WRN(TXT("Channel-count, sample-rate and bit-depth will be *ignored* for file source!\n"), m_filterSize);
		}
	}

	/*parameter range checking*/
	if((m_filterSize < 3) || (m_filterSize > 301))
	{
		PRINT2_WRN(TXT("Filter size %u is out of range. Must be in the 3 to 301 range!\n"), m_filterSize);
		return false;
	}
	if((m_filterSize % 2) != 1)
	{
		PRINT2_WRN(TXT("Filter size %u is invalid. Must be an odd value! (i.e. 'filter_size mod 2 == 1')\n"), m_filterSize);
		return false;
	}
	if((m_peakValue < 0.01) || (m_peakValue > 1.0))
	{
		PRINT2_WRN(TXT("Peak value value %.2f is out of range. Must be in the 0.01 to 1.00 range!\n"), m_peakValue);
		return false;
	}
	if((m_targetRms < 0.0) || (m_targetRms > 1.0))
	{
		PRINT2_WRN(TXT("Target RMS value %.2f is out of range. Must be in the 0.00 to 1.00 range!\n"), m_targetRms);
		return false;
	}
	if((m_compressFactor && (m_compressFactor < 1.0)) || (m_compressFactor > 30.0))
	{
		PRINT2_WRN(TXT("Compression threshold value %.2f is out of range. Must be in the 1.00 to 30.00 range!\n"), m_compressFactor);
		return false;
	}
	if((m_maxAmplification < 1.0) || (m_maxAmplification > 100.0))
	{
		PRINT2_WRN(TXT("Maximum amplification %.2f is out of range. Must be in the 1.00 to 100.00 range!\n"), m_maxAmplification);
		return false;
	}
	if((m_frameLenMsec < 10) || (m_frameLenMsec > 8000))
	{
		PRINT2_WRN(TXT("Frame length %u is out of range. Must be in the 10 to 8000 range!\n"), m_frameLenMsec);
		return false;
	}

	return true;
}
