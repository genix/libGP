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

#ifndef GPDEFINES_H
#define GPDEFINES_H

#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream> 
#include <stdlib.h>
#include <time.h>
#include <algorithm>

// ---------------------------------------------------------------------------
// GP defines / types
// ---------------------------------------------------------------------------

// maximum number of parameters the gp framework supports on a function
#define GP_MAX_PARAMETERS	3

// maximum length for naming registered functions
#define GP_DEBUGNAME_LEN	32

// maximum number of functions which may be registered with the framework
#define GP_MAX_FUNCTIONS	30

typedef long	GPTypeID;
typedef int		GPFuncID;
typedef double	GPFitness;
typedef size_t	GPHash;

const long GP_INVALID_PARAMTYPE = 0;

class	GPFunctionLookup;
class	GPFlattenedTree;
class	GPTree;
class	GPConstSubtreeIter;

GPHash GPHashString( const char* string, int size );

// since templates dont expand for each typedef, but rather for
// the types represented by the typedef, GPUNIQUE_TYPE macro
// defines a type which acts as the base type.
#define GPUNIQUE_TYPE( name, base_type ) \
class name \
{ \
public: \
	name() {} \
	name( base_type value ) : m_value( value ) {} \
	operator base_type() const { return m_value;	} \
private: \
	base_type m_value; \
};

// similar to GPUNIQUE_TYPE only with ++, -- and math operators
#define GPUNIQUE_TYPE_MATH( name, base_type ) \
class name \
{ \
public: \
	name() {} \
	name( base_type value ) : m_value( value ) {} \
	operator base_type() const { return m_value;	} \
	name& operator++() { ++m_value;	return *this; } \
	name& operator--() { --m_value;	return *this; } \
private: \
	base_type m_value; \
};

#endif // GPDEFINES_H