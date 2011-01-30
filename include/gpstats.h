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

#ifndef GPSTATS_H
#define GPSTATS_H

#include <map>
#include <vector>
#include <list>
#include <string.h>
#include "GPDefines.h"


// ---------------------------------------------------------------------------
// GPStats
//
// Quick'n'dirty stats tracker. Just set values, or push them onto a list.
// Not the main focus of this library, but a useful tool to have. Simple enough
// so no explanation :)
//
class GPStats
{
public:

	class GPStatsEntry
	{
	public:
		char m_name[GP_DEBUGNAME_LEN];

		virtual void Destroy() = 0;
	};

	template< class T >
	class GPStatsValue : public GPStatsEntry
	{
	public:
		T m_value;

		void Destroy() {}
	};

	template< class T >
	class GPStatsValueList : public GPStatsEntry
	{
	public:
		typedef std::list< T > ValueList;

		ValueList m_valuelist;

		void Destroy() {}
	};

	typedef std::map< GPHash, GPStatsEntry* > GPStatsMap;

	~GPStats();

	// TODO:
	// ensure that the name passed in is unique since its the only way to GET
	// need to register all the stats somewhere so we can check for hash clashes?
	template< class T >
	void SetSingleValue( const char* name, const T& val );

	template< class T >
	void PushListValue( const char* name, const T& val );

	void IncrementCounter( const char* name );

	//
	// TODO: gets are slow at the moment because they only work on name
	//
	template< class T >
	bool GetListValues( const char* name, typename const GPStatsValueList< T >::ValueList*& outList ) const;

private:
	GPStatsMap m_stats;

};

template< class T >
void GPStats::SetSingleValue( const char* name, const T& val )
{
	GPHash id = GPHashString( name, strlen( name ) );

	if ( m_stats.find( id ) == m_stats.end() )
	{
		GPStatsValue< T > *newStat = new GPStatsValue< T >();
		strncpy( newStat->m_name, name, GP_DEBUGNAME_LEN );
		newStat->m_value = val;

		m_stats[ id ] = newStat;
	}
	else
	{
		GPStatsValue< T > *currentStat = reinterpret_cast< GPStatsValue< T > * >( m_stats[ id ] );

		currentStat->m_value = val;
	}
}

template< class T >
void GPStats::PushListValue( const char* name, const T& val )
{
	GPHash id = GPHashString( name, strlen( name ) );

	if ( m_stats.find( id ) == m_stats.end() )
	{
		GPStatsValueList< T > *newStat = new GPStatsValueList< T >();
		strncpy( newStat->m_name, name, GP_DEBUGNAME_LEN );
		newStat->m_valuelist.push_back( val );

		m_stats[ id ] = newStat;
	}
	else
	{
		GPStatsValueList< T > *currentStat = reinterpret_cast< GPStatsValueList< T > * >( m_stats[ id ] );

		currentStat->m_valuelist.push_back( val );
	}
}

template< class T >
bool GPStats::GetListValues( const char* name, typename const GPStats::GPStatsValueList< T >::ValueList*& outList ) const
{
	GPHash id = GPHashString( name, strlen( name ) );

	// TODO: assert type check that we're getting the right type for this name
	for( GPStatsMap::const_iterator iter = m_stats.begin(); iter != m_stats.end(); ++iter )
	{
		const GPStatsEntry* thisEntry = ( *iter ).second;

		if ( ( *iter ).first == id )
		{
			const GPStatsValueList< T > *thisStat = reinterpret_cast< const GPStatsValueList< T > * >( thisEntry );

			outList = &thisStat->m_valuelist;

			return true;
		}
	}

	return false;
}


#endif // GPSTATS_H