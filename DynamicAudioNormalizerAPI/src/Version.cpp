//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

#include "Version.h"

//Version info
const unsigned int DYNAUDNORM_VERSION_MAJOR = 2;
const unsigned int DYNAUDNORM_VERSION_MINOR = 5;
const unsigned int DYNAUDNORM_VERSION_PATCH = 6;

//Build date/time
const char* DYNAUDNORM_BUILD_DATE = __DATE__;
const char* DYNAUDNORM_BUILD_TIME = __TIME__;

//Compiler detection
//Compiler detection
#if defined(__INTEL_COMPILER)
	#if (__INTEL_COMPILER >= 1300)
		const char *DYNAUDNORM_COMPILER = "ICL 13." LAMEXP_MAKE_STR(__INTEL_COMPILER_BUILD_DATE);
	#elif (__INTEL_COMPILER >= 1200)
		const char *DYNAUDNORM_COMPILER = "ICL 12." LAMEXP_MAKE_STR(__INTEL_COMPILER_BUILD_DATE);
	#elif (__INTEL_COMPILER >= 1100)
		const char *DYNAUDNORM_COMPILER = "ICL 11.x";
	#elif (__INTEL_COMPILER >= 1000)
		const char *DYNAUDNORM_COMPILER = "ICL 10.x";
	#else
		#error Compiler is not supported!
	#endif
#elif defined(_MSC_VER)
	#if (_MSC_VER == 1800)
		#if (_MSC_FULL_VER == 180021005)
			const char *DYNAUDNORM_COMPILER = "MSVC 2013";
		#elif (_MSC_FULL_VER == 180030501)
			const char *DYNAUDNORM_COMPILER = "MSVC 2013.2";
		#elif (_MSC_FULL_VER == 180030723)
			const char *DYNAUDNORM_COMPILER = "MSVC 2013.3";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1700)
		#if (_MSC_FULL_VER == 170050727)
			const char *DYNAUDNORM_COMPILER = "MSVC 2012";
		#elif (_MSC_FULL_VER == 170051106)
			const char *DYNAUDNORM_COMPILER = "MSVC 2012.1";
		#elif (_MSC_FULL_VER == 170060315)
			const char *DYNAUDNORM_COMPILER = "MSVC 2012.2";
		#elif (_MSC_FULL_VER == 170060610)
			const char *DYNAUDNORM_COMPILER = "MSVC 2012.3";
		#elif (_MSC_FULL_VER == 170061030)
			const char *DYNAUDNORM_COMPILER = "MSVC 2012.4";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1600)
		#if (_MSC_FULL_VER >= 160040219)
			const char *DYNAUDNORM_COMPILER = "MSVC 2010-SP1";
		#else
			const char *DYNAUDNORM_COMPILER = "MSVC 2010";
		#endif
	#elif (_MSC_VER == 1500)
		#if (_MSC_FULL_VER >= 150030729)
			const char *DYNAUDNORM_COMPILER = "MSVC 2008-SP1";
		#else
			const char *DYNAUDNORM_COMPILER = "MSVC 2008";
		#endif
	#else
		#error Compiler is not supported!
	#endif
	// Note: /arch:SSE and /arch:SSE2 are only available for the x86 platform
	#if !defined(_M_X64) && defined(_M_IX86_FP)
		#if (_M_IX86_FP == 1)
			#pragma message("SSE instruction set is enabled!")
		#elif (_M_IX86_FP >= 2)
			#pragma message("SSE2 (or higher) instruction set is enabled!")
		#endif
	#endif
#elif defined(__GNUC__)
	#define GCC_VERSION_GLUE1(X,Y,Z) "GCC " #X "." #Y "." #Z
	#define GCC_VERSION_GLUE2(X,Y,Z) GCC_VERSION_GLUE1(X,Y,Z)
	const char *DYNAUDNORM_COMPILER = GCC_VERSION_GLUE2(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
	#error Compiler is not supported!
#endif

//Architecture detection
#if defined(_M_X64) || defined(__x86_64__)
	const char *DYNAUDNORM_ARCH = "x64";
#elif defined(_M_IX86) || defined(__i386__)
	const char *DYNAUDNORM_ARCH = "x86";
#else
	#error Architecture is not supported!
#endif
