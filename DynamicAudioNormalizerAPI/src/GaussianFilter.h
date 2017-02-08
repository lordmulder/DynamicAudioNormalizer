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

#pragma once

#include <cstdlib>
#include <stdint.h>
#include <deque>

class MDynamicAudioNormalizer_GaussianFilter
{
public:
	MDynamicAudioNormalizer_GaussianFilter(const uint32_t &filterSize, const double &sigma);
	virtual ~MDynamicAudioNormalizer_GaussianFilter(void);

	double apply(const std::deque<double> &values);

private:
	const uint32_t m_filterSize;
	double *m_weights;

	MDynamicAudioNormalizer_GaussianFilter &operator=(const MDynamicAudioNormalizer_GaussianFilter &) { throw 666; }
};
