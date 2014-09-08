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

#pragma once

#if defined(_DEBUG) && !defined(NDEBUG)
	#define DYNAUDNORM_DEBUG (1)
#elif defined(NDEBUG) &&  !defined(_DEBUG)
	#define DYNAUDNORM_DEBUG (0)
#else
	#error Inconsistent debug defines detected!
#endif

#define MY_DELETE(X) do \
{ \
	if((X)) \
	{ \
		delete (X); \
		(X) = NULL; \
	} \
} \
while(0)

#define MY_DELETE_ARRAY(X) do \
{ \
	if((X)) \
	{ \
		delete [] (X); \
		(X) = NULL; \
	} \
} \
while(0)

#define MY_FREE(X) do \
{ \
	if((X)) \
	{ \
		free(X); \
		(X) = NULL; \
	} \
} \
while(0)

#define MY_CRITSEC_ENTER(X) do \
{ \
	if(pthread_mutex_lock(&(X)) != 0) \
	{ \
		abort(); \
	} \
} \
while(0)

#define MY_CRITSEC_LEAVE(X) do \
{ \
	if(pthread_mutex_unlock(&(X)) != 0) \
	{ \
		abort(); \
	} \
} \
while(0)
