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

#include <cstdlib>
#include <stdint.h>
#include <deque>

class GaussianFilter
{
public:
	GaussianFilter(const uint32_t &filterSize, const double &sigma);
	virtual ~GaussianFilter(void);

	void apply(double *values, const uint32_t &length, const double &defVal, const uint32_t &passes = 1);
	void apply(std::deque<double> *values, const double &defVal, const uint32_t &passes = 1);

private:
	const uint32_t m_filterSize;
	double *m_weights;
	
	double *m_temp;
	uint32_t m_tempSize;
	
	GaussianFilter &operator=(const GaussianFilter &) { throw 666; }
};
