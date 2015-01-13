//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2015 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

typedef void (DYNAUDNORM_LOG_CALLBACK)(const int logLevel, const char *const message);
DYNAUDNORM_LOG_CALLBACK * DYNAUDNORM_LOG_SETCALLBACK(DYNAUDNORM_LOG_CALLBACK *const callback);
void DYNAUDNORM_LOG_POSTMESSAGE(const int &logLevel, const char *const message, ...);

#define LOG1_DBG(X) DYNAUDNORM_LOG_POSTMESSAGE(0, (X))
#define LOG1_WRN(X) DYNAUDNORM_LOG_POSTMESSAGE(1, (X))
#define LOG1_ERR(X) DYNAUDNORM_LOG_POSTMESSAGE(2, (X))

#define LOG2_DBG(X,...) DYNAUDNORM_LOG_POSTMESSAGE(0, (X), __VA_ARGS__)
#define LOG2_WRN(X,...) DYNAUDNORM_LOG_POSTMESSAGE(1, (X), __VA_ARGS__)
#define LOG2_ERR(X,...) DYNAUDNORM_LOG_POSTMESSAGE(2, (X), __VA_ARGS__)
