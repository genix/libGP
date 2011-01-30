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

#include "gpdefines.h"
#include "gpfunctionlookup.h"

GPFuncID GPFunctionLookup::NULLFUNC = -1;

GPFuncID GPFunctionLookup::GetRandomFuncWithReturnType( GPTypeID return_type_id ) const
{
	GPFuncID startFunc = rand() % m_nFuncs;
	GPFuncID foundFunc = GetNextFuncWithReturnType( return_type_id, startFunc );

	if ( foundFunc == NULLFUNC && m_functions[ startFunc ].m_return_type == return_type_id )
		foundFunc = startFunc;

	return foundFunc;
}

GPFuncID GPFunctionLookup::GetNextFuncWithReturnType( GPTypeID return_type_id, GPFuncID previous ) const
{
	GPFuncID current = previous;
	do
	{
		current = ( current + 1 ) % m_nFuncs;
	}
	while( previous != current && m_functions[ current ].m_return_type != return_type_id );

	return ( previous != current ? current : NULLFUNC );
}

GPFuncID GPFunctionLookup::GetFunctionIDByName( const char* name, bool delayed_desired ) const
{
	for( int i = 0; i < GetNumFunctions(); ++i )
	{
		const GPFunctionDesc& this_function = GetFunctionByID( i );

		const bool is_delayed = this_function.m_original_function_id != GPFunctionLookup::NULLFUNC;

		if ( strcmp( name, this_function.m_debug_name ) == 0 && ( is_delayed && delayed_desired || !delayed_desired ) )
		{
			return i;
		}
	}

	return NULLFUNC;
}

