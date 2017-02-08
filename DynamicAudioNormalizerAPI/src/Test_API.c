//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Library
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

#include "DynamicAudioNormalizer.h"

static void MDynamicAudioNormalizer_API_Tester(void)
{
	MDynamicAudioNormalizer_Handle *handle = MDYNAMICAUDIONORMALIZER_FUNCTION(createInstance)(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, 1, 0, 0, NULL);
	if(handle)
	{
		uint32_t channels, sampleRate, frameLen, filterSize;
		MDYNAMICAUDIONORMALIZER_FUNCTION(getConfiguration)(handle, &channels, &sampleRate, &frameLen, &filterSize);
		MDYNAMICAUDIONORMALIZER_FUNCTION(destroyInstance)(&handle);
	}
}
