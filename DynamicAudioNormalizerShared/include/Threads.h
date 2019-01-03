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

#if (defined(__cplusplus) && (__cplusplus >= 201103L)) || (defined(_MSC_VER) && (_MSC_VER >= 1800))
#define HAVE_STDC11_MUTEX_SUPPORT 1
#else
#define HAVE_STDC11_MUTEX_SUPPORT 0
#endif

// ----------------------------------------------
// C++11 Mutex
// ----------------------------------------------

#if(HAVE_STDC11_MUTEX_SUPPORT)

//Std mutex
#include <mutex>

//Init
#define MY_CRITSEC_DECL(X) std::mutex X
#define MY_CRITSEC_INIT(X) std::mutex X

//Functions
#define MY_CRITSEC_ENTER(X) (X).lock()
#define MY_CRITSEC_LEAVE(X) (X).unlock()

// ----------------------------------------------
// Pthread Mutex
// ----------------------------------------------

#else //HAVE_STDC11_MUTEX_SUPPORT

//Static lib support
#if defined(_WIN32) && defined(_MT)
#define PTW32_STATIC_LIB 1
#endif

//VS2015 compile fix
#if (_MSC_VER >= 1900) && !defined(_CRT_NO_TIME_T)
#define _TIMESPEC_DEFINED 1
#endif

//PThread
#include <pthread.h>

//Init
#define MY_CRITSEC_DECL(X) pthread_mutex_t X
#define MY_CRITSEC_INIT(X) pthread_mutex_t X = PTHREAD_MUTEX_INITIALIZER

//Lock
#define MY_CRITSEC_ENTER(X) do \
{ \
	if(pthread_mutex_lock(&(X)) != 0) \
	{ \
		abort(); \
	} \
} \
while(0)

//Un-lock
#define MY_CRITSEC_LEAVE(X) do \
{ \
	if(pthread_mutex_unlock(&(X)) != 0) \
	{ \
		abort(); \
	} \
} \
while(0)

#endif //HAVE_STDC11_MUTEX_SUPPORT