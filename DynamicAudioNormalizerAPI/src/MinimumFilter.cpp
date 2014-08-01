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

#include "MinimumFilter.h"

#include "Common.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cfloat>

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

MinimumFilter::MinimumFilter(const uint32_t &filterSize)
{
}

MinimumFilter::~MinimumFilter(void)
{
}

///////////////////////////////////////////////////////////////////////////////
// Apply Filter
///////////////////////////////////////////////////////////////////////////////

double MinimumFilter::apply(const std::deque<double> &values)
{
	double minValue = DBL_MAX;
	
	for(std::deque<double>::const_iterator iter = values.begin(); iter != values.end(); iter++)
	{
		minValue = std::min(minValue, (*iter));
	}
	
	return minValue;
}
