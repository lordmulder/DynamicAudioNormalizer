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

namespace DYNAUDNORM_NS
{
	typedef void (LoggingCallback)(const int logLevel, const char *const message);
	LoggingCallback *setLoggingHandler(LoggingCallback *const callback);
	void postLogMessage(const int &logLevel, const char *const message, ...);
}

#define LOG1_DBG(X) DYNAUDNORM_NS::postLogMessage(0, (X))
#define LOG1_WRN(X) DYNAUDNORM_NS::postLogMessage(1, (X))
#define LOG1_ERR(X) DYNAUDNORM_NS::postLogMessage(2, (X))

#define LOG2_DBG(X,...) DYNAUDNORM_NS::postLogMessage(0, (X), __VA_ARGS__)
#define LOG2_WRN(X,...) DYNAUDNORM_NS::postLogMessage(1, (X), __VA_ARGS__)
#define LOG2_ERR(X,...) DYNAUDNORM_NS::postLogMessage(2, (X), __VA_ARGS__)
