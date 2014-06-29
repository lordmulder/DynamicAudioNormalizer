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

#include "MinimumFilter.h"

#include "Common.h"
#include <cmath>
#include <cstring>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// Constructor & Destructor
///////////////////////////////////////////////////////////////////////////////

MinimumFilter::MinimumFilter(const uint32_t &filterSize)
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
}

MinimumFilter::~MinimumFilter(void)
{
	MY_DELETE_ARRAY(m_temp);
}

///////////////////////////////////////////////////////////////////////////////
// Apply Filter
///////////////////////////////////////////////////////////////////////////////

void MinimumFilter::apply(double *values, const uint32_t &length, const size_t &passes)
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

	//Run minimum passes
	for(size_t pass = 0; pass < passes; pass++)
	{
		//Apply minimum filter
		for(uint32_t i = 0; i < length; i++)
		{
			double minValue = DBL_MAX;
			for(uint32_t k = i - std::min(i, offset); k < std::min(i + offset + 1, length); k++)
			{
				minValue = std::min(minValue, values[i]);
			}
			m_temp[i] = minValue;
		}

		//Copy result to output
		memcpy(values, m_temp, sizeof(double) * length);
	}
}

void MinimumFilter::apply(std::deque<double> *values, const size_t &passes)
{
	//Pre-compute constants
	const size_t length = values->size();
	const uint32_t offset = m_filterSize / 2;

	//Alocate temporary buffer
	if(m_tempSize < length)
	{
		MY_DELETE_ARRAY(m_temp);
		m_temp = new double[length];
		m_tempSize = length;
	}

	//Run minimum passes
	for(size_t pass = 0; pass < passes; pass++)
	{
		//Apply minimum filter
		for(uint32_t i = 0; i < length; i++)
		{
			double minValue = DBL_MAX;
			for(uint32_t k = i - std::min(i, offset); k < std::min(i + offset + 1, length); k++)
			{
				minValue = std::min(minValue, (*values)[k]);
			}
			m_temp[i] = minValue;
		}

		//Copy result to output
		for(uint32_t i = 0; i < length; i++)
		{
			(*values)[i] = m_temp[i];
		}
	}
}
