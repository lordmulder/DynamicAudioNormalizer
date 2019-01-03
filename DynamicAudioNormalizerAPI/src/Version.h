//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014-2019 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#pragma once

#include "Common.h"

//=============================================================================
// Version
//=============================================================================

namespace DYNAUDNORM_NS
{
	//Version info
	extern const unsigned int VERSION_MAJOR;
	extern const unsigned int VERSION_MINOR;
	extern const unsigned int VERSION_PATCH;

	//Build date/time
	extern const char *const BUILD_DATE;
	extern const char *const BUILD_TIME;

	//Compiler info
	extern const char *const BUILD_COMPILER;
	extern const char *const BUILD_ARCH;
}
