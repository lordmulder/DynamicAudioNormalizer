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

#pragma once

#include "Common.h"

class Parameters
{
public:
	Parameters(void);
	~Parameters(void);
	
	bool parseArgs(const int argc, CHR* argv[]);

	inline const CHR* sourceFile(void) const { return m_sourceFile; }
	inline const CHR* outputFile(void) const { return m_outputFile; }
	inline const CHR* dbgLogFile(void) const { return m_dbgLogFile; }
	
	inline const uint32_t &frameLenMsec(void)       const { return m_frameLenMsec;       }
	inline const uint32_t &filterSize(void)         const { return m_filterSize;         }
	inline const bool     &channelsCoupled(void)    const { return m_channelsCoupled;    }
	inline const bool     &enableDCCorrection(void) const { return m_enableDCCorrection; }
	inline const double   &peakValue(void)          const { return m_peakValue;          }
	inline const double   &maxAmplification(void)   const { return m_maxAmplification;   }
	inline const bool     &showHelp(void)           const { return m_showHelp;           }
	inline const bool     &verboseMode(void)        const { return m_verboseMode;        }

protected:
	void setDefaults(void);
	bool validateParameters(void);

private:
	const CHR* m_sourceFile;
	const CHR* m_outputFile;
	const CHR* m_dbgLogFile;

	uint32_t m_frameLenMsec;
	uint32_t m_filterSize;
	
	bool m_showHelp;
	bool m_channelsCoupled;
	bool m_enableDCCorrection;
	bool m_verboseMode;
	
	double m_peakValue;
	double m_maxAmplification;
};
