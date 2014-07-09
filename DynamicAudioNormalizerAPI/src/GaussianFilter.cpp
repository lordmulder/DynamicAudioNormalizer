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

#include "GaussianFilter.h"

#include "Common.h"

#include <cmath>
#include <cstring>
#include <stdexcept>

static const double s_pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679;

template <class T>
static inline const T& DATA(const T *data, const T &defVal, const uint32_t &count, const int32_t pos)
{
	return (pos < 0) ? defVal : ((pos >= int32_t(count)) ? defVal : data [pos]);
}

template <class T>
static inline const T& DATA(const std::deque<T> *data, const T &defVal, const uint32_t &count, const int32_t pos)
{
	return (pos < 0) ? defVal : ((pos >= int32_t(count)) ? defVal : (*data)[pos]);
}

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

GaussianFilter::GaussianFilter(const uint32_t &filterSize, const double &sigma)
:
	m_filterSize(filterSize)
{
	if((filterSize < 1) || ((filterSize % 2) != 1))
	{
		throw std::runtime_error("Filter size must be a positive and odd value!");
	}
	
	//Init temp buffer;
	m_temp = NULL;
	m_tempSize = 0;

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

GaussianFilter::~GaussianFilter(void)
{
	MY_DELETE_ARRAY(m_weights);
	MY_DELETE_ARRAY(m_temp);
}

///////////////////////////////////////////////////////////////////////////////
// Apply Filter
///////////////////////////////////////////////////////////////////////////////

void GaussianFilter::apply(double *values, const uint32_t &length, const double &defVal, const uint32_t &passes)
{
	//Pre-compute constants
	const uint32_t offset = m_filterSize / 2;

	//Alocate temporary buffer
	if(m_tempSize < length)
	{
		MY_DELETE_ARRAY(m_temp);
		m_temp = new double[length];
		m_tempSize = length;
	}

	//Run smoothing passes
	for(uint32_t pass = 0; pass < passes; pass++)
	{
		//Apply Gaussian filter
		for(uint32_t i = 0; i < length; i++)
		{
			m_temp[i] = 0.0; uint32_t w = 0;
			for(int32_t k = int32_t(i) - int32_t(offset); w < m_filterSize; k++)
			{
				m_temp[i] += DATA(values, defVal, length, k) * m_weights[w++];
			}
		}

		//Copy result to output
		memcpy(values, m_temp, sizeof(double) * length);
	}
}

void GaussianFilter::apply(std::deque<double> *values, const double &defVal, const uint32_t &passes)
{
	//Pre-compute constants
	const uint32_t length = values->size();
	const uint32_t offset = m_filterSize / 2;

	//Alocate temporary buffer
	if(m_tempSize < length)
	{
		MY_DELETE_ARRAY(m_temp);
		m_temp = new double[length];
		m_tempSize = length;
	}

	//Run smoothing passes
	for(uint32_t pass = 0; pass < passes; pass++)
	{
		//Apply Gaussian filter
		for(uint32_t i = 0; i < length; i++)
		{
			m_temp[i] = 0.0; uint32_t w = 0;
			for(int32_t k = int32_t(i) - int32_t(offset); w < m_filterSize; k++)
			{
				m_temp[i] += DATA(values, defVal, length, k) * m_weights[w++];
			}
		}

		//Copy result to output
		for(uint32_t i = 0; i < length; i++)
		{
			(*values)[i] = m_temp[i];
		}
	}
}
