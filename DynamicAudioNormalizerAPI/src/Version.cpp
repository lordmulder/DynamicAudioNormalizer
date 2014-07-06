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

#include "Version.h"

//Version info
const unsigned int DYAUNO_VERSION_MAJOR = 1;
const unsigned int DYAUNO_VERSION_MINOR = 1;
const unsigned int DYAUNO_VERSION_PATCH = 9;

//Build date/time
const char* DYAUNO_BUILD_DATE = __DATE__;
const char* DYAUNO_BUILD_TIME = __TIME__;

//Compiler detection
#if defined(__INTEL_COMPILER)
	#if (__INTEL_COMPILER >= 1300)
		static char *DYAUNO_COMPILER = "ICL 13.x";
	#elif (__INTEL_COMPILER >= 1200)
		static char *DYAUNO_COMPILER = "ICL 12x.";
	#elif (__INTEL_COMPILER >= 1100)
		static char *DYAUNO_COMPILER = "ICL 11.x";
	#elif (__INTEL_COMPILER >= 1000)
		static char *DYAUNO_COMPILER = "ICL 10.x";
	#else
		#error Compiler is not supported!
	#endif
#elif defined(_MSC_VER)
	#if (_MSC_VER == 1800)
		#if (_MSC_FULL_VER < 180021005)
			const char *DYAUNO_COMPILER = "MSVC 2013-Beta";
		#elif (_MSC_FULL_VER < 180030501)
			const char *DYAUNO_COMPILER = "MSVC 2013";
		#elif (_MSC_FULL_VER == 180030501)
			const char *DYAUNO_COMPILER = "MSVC 2013.2";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1700)
		#if (_MSC_FULL_VER < 170050727)
			const char *DYAUNO_COMPILER = "MSVC 2012-Beta";
		#elif (_MSC_FULL_VER < 170051020)
			const char *DYAUNO_COMPILER = "MSVC 2012");
		#elif (_MSC_FULL_VER < 170051106)
			const char *DYAUNO_COMPILER = "MSVC 2012.1-CTP";
		#elif (_MSC_FULL_VER < 170060315)
			const char *DYAUNO_COMPILER = "MSVC 2012.1";
		#elif (_MSC_FULL_VER < 170060610)
			const char *DYAUNO_COMPILER = "MSVC 2012.2";
		#elif (_MSC_FULL_VER < 170061030)
			const char *DYAUNO_COMPILER = "MSVC 2012.3";
		#elif (_MSC_FULL_VER == 170061030)
			const char *DYAUNO_COMPILER = "MSVC 2012.4";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1600)
		#if (_MSC_FULL_VER < 160040219)
			const char *DYAUNO_COMPILER = "MSVC 2010";
		#elif (_MSC_FULL_VER == 160040219)
			const char *DYAUNO_COMPILER = "MSVC 2010-SP1";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1500)
		#if (_MSC_FULL_VER >= 150030729)
			const char *DYAUNO_COMPILER = "MSVC 2008-SP1";
		#else
			const char *DYAUNO_COMPILER = "MSVC 2008";
		#endif
	#else
		#error Compiler is not supported!
	#endif

	// Note: /arch:SSE and /arch:SSE2 are only available for the x86 platform
	#if !defined(_M_X64) && defined(_M_IX86_FP)
		#if (_M_IX86_FP == 1)
			#pragma message("SSE instruction set is enabled!")
		#elif (_M_IX86_FP == 2)
			#pragma message("SSE2 (or higher) instruction set is enabled!")
		#endif
	#endif
#else
	#error Compiler is not supported!
#endif

//Architecture detection
#if defined(_M_X64)
	const char *DYAUNO_ARCH = "x64";
#elif defined(_M_IX86)
	const char *DYAUNO_ARCH = "x86";
#else
	#error Architecture is not supported!
#endif
