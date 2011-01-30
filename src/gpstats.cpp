/*
 * This source file is part of libGP C++ library.
 * 
 * Copyright (c) 2011 Craig Furness
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "GPStats.h"

GPStats::~GPStats()
{
	GPStatsMap::iterator iter = m_stats.begin();
	while( iter != m_stats.end() )
	{
		iter->second->Destroy();
		delete iter->second;
		++iter;
	}

	m_stats.clear();
}

void GPStats::IncrementCounter( const char* name )
{
	GPHash id = GPHashString( name, strlen( name ) );

	if ( m_stats.find( id ) == m_stats.end() )
	{
		GPStatsValue< int > *newStat = new GPStatsValue< int >();
		strncpy( newStat->m_name, name, GP_DEBUGNAME_LEN );
		newStat->m_value = 1;

		m_stats[ id ] = newStat;
	}
	else
	{
		GPStatsValue< int > *currentStat = reinterpret_cast< GPStatsValue< int > * >( m_stats[ id ] );

		++currentStat->m_value;
	}
}
