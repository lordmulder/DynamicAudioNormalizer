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

#if INC_DYNAUDNORM_VERSION_INTERNAL != 0x22b4f8a9
#error "Please do *not* include this file directly; include 'Version.h' instead!"
#endif

//////////////////////////////////////////////////////////////////////////////////
// Version Info
//////////////////////////////////////////////////////////////////////////////////

#define VER_DYNAUDNORM_MAJOR      2
#define VER_DYNAUDNORM_MINOR_HI   1
#define VER_DYNAUDNORM_MINOR_LO   1
#define VER_DYNAUDNORM_PATCH      1

//////////////////////////////////////////////////////////////////////////////////
// Helper macros (aka: having fun with the C pre-processor)
//////////////////////////////////////////////////////////////////////////////////

#define ___VER_DYNAUDNORM_STR___(X)       #X
#define __VER_DYNAUDNORM_STR__(W,X,Y,Z)   ___VER_DYNAUDNORM_STR___(v##W.X##Y-Z)
#define _VER_DYNAUDNORM_STR_(W,X,Y,Z)     __VER_DYNAUDNORM_STR__(W,X,Y,Z)
#define VER_DYNAUDNORM_STR                _VER_DYNAUDNORM_STR_(VER_DYNAUDNORM_MAJOR,VER_DYNAUDNORM_MINOR_HI,VER_DYNAUDNORM_MINOR_LO,VER_DYNAUDNORM_PATCH)

#define __VER_DYNAUDNORM_MINOR__(X,Y)     ((10U * (X)) + (Y))
#define _VER_DYNAUDNORM_MINOR_(X,Y)       __VER_DYNAUDNORM_MINOR__(X,Y)
#define VER_DYNAUDNORM_MINOR              _VER_DYNAUDNORM_MINOR_(VER_DYNAUDNORM_MINOR_HI,VER_DYNAUDNORM_MINOR_LO)
