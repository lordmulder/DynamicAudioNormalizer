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

typedef void (DYNAUDNORM_LOG_CALLBACK)(const int logLevel, const char *const message);
DYNAUDNORM_LOG_CALLBACK * DYNAUDNORM_LOG_SETCALLBACK(DYNAUDNORM_LOG_CALLBACK *const callback);
void DYNAUDNORM_LOG_POSTMESSAGE(const int &logLevel, const char *const message, ...);

#define LOG_DBG(X) DYNAUDNORM_LOG_POSTMESSAGE(0, (X))
#define LOG_WRN(X) DYNAUDNORM_LOG_POSTMESSAGE(1, (X))
#define LOG_ERR(X) DYNAUDNORM_LOG_POSTMESSAGE(2, (X))

#define LOG2_DBG(X,...) DYNAUDNORM_LOG_POSTMESSAGE(0, (X), __VA_ARGS__)
#define LOG2_WRN(X,...) DYNAUDNORM_LOG_POSTMESSAGE(1, (X), __VA_ARGS__)
#define LOG2_ERR(X,...) DYNAUDNORM_LOG_POSTMESSAGE(2, (X), __VA_ARGS__)
