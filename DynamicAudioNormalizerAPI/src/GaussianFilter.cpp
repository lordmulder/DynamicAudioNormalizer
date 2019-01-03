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

#include "GaussianFilter.h"

#include "Common.h"

#include <cmath>
#include <cstring>
#include <stdexcept>

static const double s_pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679;

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

DYNAUDNORM_NS::GaussianFilter::GaussianFilter(const uint32_t &filterSize, const double &sigma)
:
	m_filterSize(filterSize)
{
	if((filterSize < 1) || ((filterSize % 2) != 1))
	{
		throw std::runtime_error("Filter size must be a positive and odd value!");
	}
	
	//Allocate weights
	m_weights = new double[filterSize];
	double totalWeight = 0.0;

	//Pre-computer constants
	const uint32_t offset = m_filterSize / 2;
	const double c1 = 1.0 / (sigma * sqrt(2.0 * s_pi));
	const double c2 = 2.0 * pow(sigma, 2.0);

	//Compute weights
	for(uint32_t i = 0; i < m_filterSize; i++)
	{
		const int32_t x = int32_t(i) - int32_t(offset);
		m_weights[i] = c1 * exp(-(pow(x, 2.0) / c2));
		totalWeight += m_weights[i];
	}

	//Adjust weights
	const double adjust = 1.0 / totalWeight;
	for(uint32_t i = 0; i < m_filterSize; i++)
	{
		m_weights[i] *= adjust;
		//LOG_WRN(TXT("Gauss Weight %02u = %.4f"), i, m_weights[i]);
	}
}

DYNAUDNORM_NS::GaussianFilter::~GaussianFilter(void)
{
	MY_DELETE_ARRAY(m_weights);
}

///////////////////////////////////////////////////////////////////////////////
// Apply Filter
///////////////////////////////////////////////////////////////////////////////

double DYNAUDNORM_NS::GaussianFilter::apply(const std::deque<double> &values)
{
	if(values.size() != m_filterSize)
	{
		throw std::runtime_error("Input data has the wrong size!");
	}
	
	uint32_t w = 0;
	double result = 0.0;

	//Apply Gaussian filter
	for(std::deque<double>::const_iterator iter = values.begin(); iter != values.end(); iter++)
	{
		result += (*iter) * m_weights[w++];
	}

	return result;
}
