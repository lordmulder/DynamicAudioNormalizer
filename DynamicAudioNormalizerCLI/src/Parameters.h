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

#pragma once

#include "Common.h"
#include "Platform.h"

class Parameters
{
public:
	Parameters(void);
	~Parameters(void);
	
	bool parseArgs(const int argc, CHR* argv[]);

	inline const CHR* sourceFile(void)    const { return m_sourceFile;    }
	inline const CHR* sourceLibrary(void) const { return m_sourceLibrary; }
	inline const CHR* outputFile(void)    const { return m_outputFile;    }
	inline const CHR* outputFormat(void)  const { return m_outputFormat;  }
	inline const CHR* dbgLogFile(void)    const { return m_dbgLogFile;    }
	
	inline const uint32_t &frameLenMsec(void)       const { return m_frameLenMsec;       }
	inline const uint32_t &filterSize(void)         const { return m_filterSize;         }
	inline const uint32_t &inputChannels(void)      const { return m_inputChannels;      }
	inline const uint32_t &inputSampleRate(void)    const { return m_inputSampleRate;    }
	inline const uint32_t &inputBitDepth(void)      const { return m_inputBitDepth;      }
	inline const uint32_t &outputBitDepth(void)     const { return m_outputBitDepth;     }
	inline const double   &peakValue(void)          const { return m_peakValue;          }
	inline const double   &maxAmplification(void)   const { return m_maxAmplification;   }
	inline const double   &targetRms(void)          const { return m_targetRms;          }
	inline const double   &compressFactor(void)     const { return m_compressFactor;     }
	inline const bool     &channelsCoupled(void)    const { return m_channelsCoupled;    }
	inline const bool     &enableDCCorrection(void) const { return m_enableDCCorrection; }
	inline const bool     &altBoundaryMode(void)    const { return m_altBoundaryMode;    }
	inline const bool     &showHelp(void)           const { return m_showHelp;           }
	inline const bool     &verboseMode(void)        const { return m_verboseMode;        }

protected:
	void setDefaults(void);
	bool validateParameters(void);

private:
	const CHR* m_sourceFile;
	const CHR* m_sourceLibrary;
	const CHR* m_outputFile;
	const CHR* m_outputFormat;
	const CHR* m_dbgLogFile;

	uint32_t m_frameLenMsec;
	uint32_t m_filterSize;
	uint32_t m_inputChannels;
	uint32_t m_inputSampleRate;
	uint32_t m_inputBitDepth;
	uint32_t m_outputBitDepth;

	bool m_channelsCoupled;
	bool m_enableDCCorrection;
	bool m_altBoundaryMode;
	bool m_verboseMode;
	bool m_showHelp;
	
	double m_peakValue;
	double m_maxAmplification;
	double m_targetRms;
	double m_compressFactor;
};
