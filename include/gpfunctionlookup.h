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

#ifndef GPFUNCTIONLOOKUP_H
#define GPFUNCTIONLOOKUP_H

#include <string.h>
#include "gptree.h"

class GPMemberPointerExamples
{
public:
	virtual void VirtualMember();
};

#define GPVIRTUALMEMBERSIZE sizeof( &GPMemberPointerExamples::VirtualMember )

// ---------------------------------------------------------------------------
// GPFunctionDescType
//
// Stores information about a C++ function registered to be used as a node
// in a GP. The particulars including what parameters it takes are stored.
//
typedef struct GPFunctionDescType
{
	// pointer to actual function which will be called
	char m_function_ptr[ GPVIRTUALMEMBERSIZE ];

	// pointer to function which will invoke the required function
	// the invoke function should only require 1 parameter, and return void
	uintptr_t m_invoke_ptr;

	// if this GPFunctionDescType represents a member function, 
	// this is a pointer to the owning class of said member function.
	uintptr_t m_member_owner;

	// number of parameters this function uses
	int m_nparams;

	// internal 'type id' for the returned value
	GPTypeID m_return_type;

	// parameter 'type ids' - only those the function uses are set
	GPTypeID m_param_types[ GP_MAX_PARAMETERS ];

	// if this function is a "delayed execution" version of another function
	// then the m_invoke_ptr will be returning a GPDelayedEvaluation construct
	// which will need to know what the ID of the "original" function
	// we will be delaying is.
	GPFuncID m_original_function_id;

	char m_debug_name[GP_DEBUGNAME_LEN];

	GPFunctionDescType()
	{
		memset( m_function_ptr, 0, GPVIRTUALMEMBERSIZE );

		m_invoke_ptr		= NULL;
		m_nparams			= 0;
		m_return_type		= GP_INVALID_PARAMTYPE;
		m_member_owner		= NULL;
		m_debug_name[0]		= '\0';

		m_original_function_id = -1;

		for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
		{
			m_param_types[ i ] = GP_INVALID_PARAMTYPE;
		}
	}
} GPFunctionDesc;


// ---------------------------------------------------------------------------
// GPFunctionLookup
//
// A registry for C++ functions. Allows some lookups such as being able
// to find a function with a specified return type.
//
class GPFunctionLookup
{
public:
	static GPFuncID NULLFUNC;

	GPFunctionLookup()
	{
		m_nFuncs = 0;
	}

	// 0 params
	template< class R >
		GPFuncID RegisterFunction( const char* name, R (*myFunc)() );
	template< class C, class R >
		GPFuncID RegisterFunction( const char* name, const C* owner, R (C::*myFunc)() );

	// 1 param
	template< class R, class P1 >
		GPFuncID RegisterFunction( const char* name, R (*myFunc)( P1 ) );
	template< class C, class R, class P1 >
		GPFuncID RegisterFunction( const char* name, const C* owner, R (C::*myFunc)( P1 ) );

	// 2 params
	template< class R, class P1, class P2 >
		GPFuncID RegisterFunction( const char* name, R (*myFunc)( P1, P2 ) );
	template< class C, class R, class P1, class P2 >
		GPFuncID RegisterFunction( const char* name, const C* owner, R (C::*myFunc)( P1, P2 ) );

	// 3 params
	template< class R, class P1, class P2, class P3 >
		GPFuncID RegisterFunction( const char* name, R (*myFunc)( P1, P2, P3 ) );
	template< class C, class R, class P1, class P2, class P3 >
		GPFuncID RegisterFunction( const char* name, const C* owner, R (C::*myFunc)( P1, P2, P3 ) );

	const bool FunctionIDExists( const GPFuncID id ) const;

	const GPFunctionDesc& GetFunctionByID( const GPFuncID id ) const
	{
		return m_functions[ id ];
	}

	// find a function by name (optionally request the delayed version)
	GPFuncID GetFunctionIDByName( const char* name, bool delayed_desired = false ) const;

	GPFuncID GetRandomFuncWithReturnType( GPTypeID return_type_id ) const;
	GPFuncID GetNextFuncWithReturnType( GPTypeID return_type_id, GPFuncID previous ) const;


private:

	int GetNumFunctions() const
	{
		return m_nFuncs;
	}

	int m_nFuncs;
	GPFunctionDesc m_functions[ GP_MAX_FUNCTIONS * 2 ]; // double because there will be a 'delayed' version of each function too

};

inline const bool GPFunctionLookup::FunctionIDExists( const GPFuncID id ) const
{
	return id < GetNumFunctions();
}

// ---------------------------------------------------------------------------
// GPDelayedEvaluation
//
// In the cases where a GP node doesnt want or need all of its parameters
// calculated, it can specify the parameters to be a GPDelayedEvaluation
// which lets the GP node choose when it would like to calculate the
// value of that parameter.
//
// Useful for functions which are conditionals, and want to choose which
// branch to execute - but not have both execute.
//
template< class R >
class GPDelayedEvaluation
{
public:
	GPDelayedEvaluation( const GPFunctionLookup& functions, const GPTreeNode& f  )
		: m_functions( functions ), m_treenode( f )
	{ }

	R Evaluate() const;
private:
	const GPFunctionLookup& m_functions;
	const GPTreeNode&		m_treenode;
};

template< class R >
R GPDelayedEvaluation< R >::Evaluate() const
{
	typedef R(*InvokeFuncSignature)( const GPFunctionLookup& functions, const GPTreeNode& f);

	GPFuncID original_function_id = m_functions.GetFunctionByID( m_treenode.functionID ).m_original_function_id;
	InvokeFuncSignature invoke_func = reinterpret_cast< InvokeFuncSignature >( m_functions.GetFunctionByID( original_function_id ).m_invoke_ptr );
	return invoke_func( m_functions, m_treenode );
}

template< class P >
GPTypeID GPGetTypeID()
{
	static int returnMarker = 0;
	return (GPTypeID)&returnMarker;
}

template< class R >
GPDelayedEvaluation< R > GPDelayedInvokeFunction( const GPFunctionLookup& functions, const GPTreeNode& f )
{
	return GPDelayedEvaluation< R >( functions, f );
}

// ---------------------------------------------------------------------------
// GPInvokeFunction and GPInvokeMemberFunction
//
// The invoke functions are essentially wrappers. When a C++ function is
// registered with the GPFunctionLookup to be used as a node in a GP,
// we need a way to call that function without needing to know its signature.
//
// The GPInvokeFunction has standard parameters, but internally will call the
// actual C++ function with the correct parameters.
//
// The GPInvokeMemberFunction is a similar wrapper, but its able to call member
// functions on classes.
//
// ---------------------------------------------------------------------------

template< class R >
R GPInvokeFunction0( const GPFunctionLookup& functions, const GPTreeNode& f )
{
	typedef R(*WrappedFunctionSignature)();
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, functions.GetFunctionByID( f.functionID ).m_function_ptr, sizeof( WrappedFunctionSignature ) );
	return function_ptr();
}

template< class C, class R >
R GPInvokeMemberFunction0( const GPFunctionLookup& functions, const GPTreeNode& f )
{
	typedef R(C::*WrappedFunctionSignature)();
	const GPFunctionDesc& desc = functions.GetFunctionByID( f.functionID );
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, desc.m_function_ptr, sizeof( WrappedFunctionSignature ) );
	C* classptr = reinterpret_cast< C* >( desc.m_member_owner );
	return (classptr->*function_ptr)();
}

template< class R, class P1 >
R GPInvokeFunction1( const GPFunctionLookup& functions, const GPTreeNode& f)
{
	typedef R(*WrappedFunctionSignature)(P1);
	typedef P1(*Param1InvokeSignature)( const GPFunctionLookup& , const GPTreeNode& );
	const GPFunctionDesc& desc = functions.GetFunctionByID( f.functionID );
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, desc.m_function_ptr, sizeof( WrappedFunctionSignature ) );
	Param1InvokeSignature param1Func = reinterpret_cast< Param1InvokeSignature >( functions.GetFunctionByID( f.parameters[0]->functionID ).m_invoke_ptr );
	return function_ptr
		( 
			param1Func( functions, *(f.parameters[0]) )
		);
}

template< class C, class R, class P1 >
R GPInvokeMemberFunction1( const GPFunctionLookup& functions, const GPTreeNode& f)
{
	typedef R(C::*WrappedFunctionSignature)(P1);
	typedef P1(*Param1InvokeSignature)( const GPFunctionLookup& , const GPTreeNode& );
	const GPFunctionDesc& desc = functions.GetFunctionByID( f.functionID );
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, desc.m_function_ptr, sizeof( WrappedFunctionSignature ) );
	Param1InvokeSignature param1Func = reinterpret_cast< Param1InvokeSignature >( functions.GetFunctionByID( f.parameters[0]->functionID ).m_invoke_ptr );
	C* classptr = reinterpret_cast< C* >( desc.m_member_owner );
	return (classptr->*function_ptr)
		( 
			param1Func( functions, *(f.parameters[0]) )
		);
}

template< class R, class P1, class P2 >
R GPInvokeFunction2( const GPFunctionLookup& functions, const GPTreeNode& f )
{
	typedef P1(*Param1InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef P2(*Param2InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R(*WrappedFunctionSignature)(P1,P2);
	const GPFunctionDesc& desc = functions.GetFunctionByID( f.functionID );
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, desc.m_function_ptr, sizeof( WrappedFunctionSignature ) );
	Param1InvokeSignature param1Func = reinterpret_cast< Param1InvokeSignature >( functions.GetFunctionByID( f.parameters[0]->functionID ).m_invoke_ptr );
	Param2InvokeSignature param2Func = reinterpret_cast< Param2InvokeSignature >( functions.GetFunctionByID( f.parameters[1]->functionID ).m_invoke_ptr );
	return function_ptr
		( 
			param1Func( functions, *(f.parameters[0]) ), 
			param2Func( functions, *(f.parameters[1]) )
		);
}

template< class C, class R, class P1, class P2 >
R GPInvokeMemberFunction2( const GPFunctionLookup& functions, const GPTreeNode& f )
{
	typedef P1(*Param1InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef P2(*Param2InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R(C::*WrappedFunctionSignature)(P1,P2);
	const GPFunctionDesc& desc = functions.GetFunctionByID( f.functionID );
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, desc.m_function_ptr, sizeof( WrappedFunctionSignature ) );
	Param1InvokeSignature param1Func = reinterpret_cast< Param1InvokeSignature >( functions.GetFunctionByID( f.parameters[0]->functionID ).m_invoke_ptr );
	Param2InvokeSignature param2Func = reinterpret_cast< Param2InvokeSignature >( functions.GetFunctionByID( f.parameters[1]->functionID ).m_invoke_ptr );
	C* classptr = reinterpret_cast< C* >( desc.m_member_owner );
	return (classptr->*function_ptr)
		( 
			param1Func( functions, *(f.parameters[0]) ), 
			param2Func( functions, *(f.parameters[1]) )
		);
}
template< class R, class P1, class P2, class P3 >
R GPInvokeFunction3( const GPFunctionLookup& functions, const GPTreeNode& f )
{
	typedef P1(*Param1InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef P2(*Param2InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef P3(*Param3InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R(*WrappedFunctionSignature)(P1,P2,P3);
	const GPFunctionDesc& desc = functions.GetFunctionByID( f.functionID );
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, desc.m_function_ptr, sizeof( WrappedFunctionSignature ) );
	Param1InvokeSignature param1Func = reinterpret_cast< Param1InvokeSignature >( functions.GetFunctionByID( f.parameters[0]->functionID ).m_invoke_ptr );
	Param2InvokeSignature param2Func = reinterpret_cast< Param2InvokeSignature >( functions.GetFunctionByID( f.parameters[1]->functionID ).m_invoke_ptr );
	Param3InvokeSignature param3Func = reinterpret_cast< Param3InvokeSignature >( functions.GetFunctionByID( f.parameters[2]->functionID ).m_invoke_ptr );
	return function_ptr
		( 
			param1Func( functions, *(f.parameters[0]) ), 
			param2Func( functions, *(f.parameters[1]) ), 
			param3Func( functions, *(f.parameters[2]) )
		);
}

template< class C, class R, class P1, class P2, class P3 >
R GPInvokeMemberFunction3( const GPFunctionLookup& functions, const GPTreeNode& f )
{
	typedef P1(*Param1InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef P2(*Param2InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef P3(*Param3InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R(C::*WrappedFunctionSignature)(P1,P2,P3);
	const GPFunctionDesc& desc = functions.GetFunctionByID( f.functionID );
	WrappedFunctionSignature function_ptr;
	memcpy( &function_ptr, desc.m_function_ptr, sizeof( WrappedFunctionSignature ) );
	Param1InvokeSignature param1Func = reinterpret_cast< Param1InvokeSignature >( functions.GetFunctionByID( f.parameters[0]->functionID ).m_invoke_ptr );
	Param2InvokeSignature param2Func = reinterpret_cast< Param2InvokeSignature >( functions.GetFunctionByID( f.parameters[1]->functionID ).m_invoke_ptr );
	Param3InvokeSignature param3Func = reinterpret_cast< Param3InvokeSignature >( functions.GetFunctionByID( f.parameters[2]->functionID ).m_invoke_ptr );
	C* classptr = reinterpret_cast< C* >( desc.m_member_owner );
	return (classptr->*function_ptr)
		( 
			param1Func( functions, *(f.parameters[0]) ), 
			param2Func( functions, *(f.parameters[1]) ), 
			param3Func( functions, *(f.parameters[2]) )
		);
}

template< class R >
R ExecuteTree( const GPFunctionLookup& functions, const GPTreeNode* treeRoot )
{
	typedef R(*InvokeFuncSignature)( const GPFunctionLookup& functions, const GPTreeNode& f);

	// test call the invoke
	InvokeFuncSignature invoke_func = reinterpret_cast< InvokeFuncSignature >( functions.GetFunctionByID( treeRoot->functionID ).m_invoke_ptr );
	return invoke_func( functions, *treeRoot );
}

template< class R >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, R (*myFunc)() )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (*ActualSignature)();

	GPTypeID return_type = GPGetTypeID< R >();

	InvokeSignature			invoke_func			= &( GPInvokeFunction0< R > );
	DelayedInvokeSignature	delayed_invoke_func	= &( GPDelayedInvokeFunction< R > );
	ActualSignature			original_func		= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( ActualSignature ) );
	memcpy( finfo.m_function_ptr, &original_func, sizeof( ActualSignature ) );

	finfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type	= return_type;
	finfo.m_nparams		= 0;

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type	= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add the two functions to the table
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}


template< class C, class R >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, const C* owner, R (C::*myFunc)() )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (C::*ActualSignature)();

	GPTypeID return_type = GPGetTypeID< R >();

	InvokeSignature			invoke_func			= &( GPInvokeMemberFunction0< C, R > );
	DelayedInvokeSignature	delayed_invoke_func	= &( GPDelayedInvokeFunction< R > );
	ActualSignature			actualFunc			= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( ActualSignature ) );
	memcpy( finfo.m_function_ptr, &actualFunc, sizeof( ActualSignature ) );

	finfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type	= return_type;
	finfo.m_nparams		= 0;
	finfo.m_member_owner = reinterpret_cast< uintptr_t >( owner );

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type	= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add the two functions to the table
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}

template< class R, class P1 >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, R (*myFunc)( P1 ) )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (*ActualSignature)( P1 );

	GPTypeID	return_type			= GPGetTypeID< R >();
	GPTypeID	p1Type				= GPGetTypeID< P1 >();
	InvokeSignature invoke_func		= &( GPInvokeFunction1< R, P1 > );
	DelayedInvokeSignature delayed_invoke_func = &( GPDelayedInvokeFunction< R > );
	ActualSignature	original_func	= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( ActualSignature ) );
	memcpy( finfo.m_function_ptr, &original_func, sizeof( ActualSignature ) );

	finfo.m_invoke_ptr		= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type		= return_type;
	finfo.m_param_types[0]	= p1Type;
	finfo.m_nparams			= 1;

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type	= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add it to the list of known function in the environment
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}

template< class C, class R, class P1 >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, const C* owner, R (C::*myFunc)( P1 ) )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (C::*ActualSignature)( P1 );

	GPTypeID return_type	= GPGetTypeID< R >();
	GPTypeID p1Type		= GPGetTypeID< P1 >();

	InvokeSignature			invoke_func			= &GPInvokeMemberFunction1< C, R, P1 >;
	DelayedInvokeSignature	delayed_invoke_func	= &GPDelayedInvokeFunction< R >;
	ActualSignature			original_func		= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( ActualSignature ) );
	memcpy( finfo.m_function_ptr, &original_func, sizeof( ActualSignature ) );

	finfo.m_invoke_ptr		= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type		= return_type;
	finfo.m_param_types[0]	= p1Type;
	finfo.m_nparams			= 1;
	finfo.m_member_owner	= reinterpret_cast< uintptr_t >( owner );

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type	= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add it to the list of known function in the environment
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}

template< class R, class P1, class P2 >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, R (*myFunc)( P1, P2 ) )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (*ActualSignature)( P1, P2 );

	GPTypeID return_type = GPGetTypeID< R >();
	GPTypeID p1Type = GPGetTypeID< P1 >();
	GPTypeID p2Type = GPGetTypeID< P2 >();

	InvokeSignature			invoke_func			= &( GPInvokeFunction2< R, P1, P2 > );
	DelayedInvokeSignature	delayed_invoke_func	= &( GPDelayedInvokeFunction< R > );
	ActualSignature			original_func		= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( original_func ) );
	memcpy( finfo.m_function_ptr, &original_func, sizeof( original_func ) );

	finfo.m_invoke_ptr		= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type		= return_type;
	finfo.m_param_types[0]	= p1Type;
	finfo.m_param_types[1]	= p2Type;
	finfo.m_nparams			= 2;

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type		= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add it to the list of known function in the environment
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}

template< class C, class R, class P1, class P2 >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, const C* owner, R (C::*myFunc)( P1, P2 ) )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (C::*ActualSignature)( P1, P2 );

	GPTypeID return_type = GPGetTypeID< R >();
	GPTypeID p1Type = GPGetTypeID< P1 >();
	GPTypeID p2Type = GPGetTypeID< P2 >();

	InvokeSignature			invoke_func			= &( GPInvokeMemberFunction2< C, R, P1, P2 > );
	DelayedInvokeSignature	delayed_invoke_func	= &( GPDelayedInvokeFunction< R > );
	ActualSignature			original_func		= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( original_func ) );
	memcpy( finfo.m_function_ptr, &original_func, sizeof( ActualSignature ) );

	finfo.m_invoke_ptr		= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type		= return_type;
	finfo.m_param_types[0]	= p1Type;
	finfo.m_param_types[1]	= p2Type;
	finfo.m_nparams			= 2;
	finfo.m_member_owner	= reinterpret_cast< uintptr_t >( owner );

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type	= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add it to the list of known function in the environment
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}

template< class R, class P1, class P2, class P3 >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, R (*myFunc)( P1, P2, P3 ) )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (*ActualSignature)( P1, P2, P3 );

	GPTypeID return_type = GPGetTypeID< R >();
	GPTypeID p1Type = GPGetTypeID< P1 >();
	GPTypeID p2Type = GPGetTypeID< P2 >();
	GPTypeID p3Type = GPGetTypeID< P3 >();

	InvokeSignature			invoke_func			= &( GPInvokeFunction3< R, P1, P2, P3 > );
	DelayedInvokeSignature delayed_invoke_func	= &( GPDelayedInvokeFunction< R > );
	ActualSignature			original_func		= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( original_func ) );
	memcpy( finfo.m_function_ptr, &original_func, sizeof( ActualSignature ) );

	finfo.m_invoke_ptr		= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type		= return_type;
	finfo.m_param_types[0]	= p1Type;
	finfo.m_param_types[1]	= p2Type;
	finfo.m_param_types[2]	= p3Type;
	finfo.m_nparams			= 3;

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type	= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add it to the list of known function in the environment
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}

template< class C, class R, class P1, class P2, class P3 >
GPFuncID GPFunctionLookup::RegisterFunction( const char* name, const C* owner, R (C::*myFunc)( P1, P2, P3 ) )
{
	typedef R (*InvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef GPDelayedEvaluation< R > (*DelayedInvokeSignature)(const GPFunctionLookup& , const GPTreeNode&);
	typedef R (C::*ActualSignature)( P1, P2, P3 );

	GPTypeID return_type = GPGetTypeID< R >();
	GPTypeID p1Type = GPGetTypeID< P1 >();
	GPTypeID p2Type = GPGetTypeID< P2 >();
	GPTypeID p3Type = GPGetTypeID< P3 >();

	InvokeSignature			invoke_func			= &( GPInvokeMemberFunction3< C, R, P1, P2, P3 > );
	DelayedInvokeSignature	delayed_invoke_func	= &( GPDelayedInvokeFunction< R > );
	ActualSignature			actualFunc			= myFunc;

	// fill out a function info for this
	GPFunctionDesc finfo, delayedfinfo;

	assert( GPVIRTUALMEMBERSIZE >= sizeof( actualFunc ) );
	memcpy( finfo.m_function_ptr, &actualFunc, sizeof( ActualSignature ) );

	finfo.m_invoke_ptr		= reinterpret_cast< uintptr_t >( invoke_func );
	finfo.m_return_type		= return_type;
	finfo.m_param_types[0]	= p1Type;
	finfo.m_param_types[1]	= p2Type;
	finfo.m_param_types[2]	= p3Type;
	finfo.m_nparams			= 3;
	finfo.m_member_owner	= reinterpret_cast< uintptr_t >( owner );

	strncpy( finfo.m_debug_name, name, GP_DEBUGNAME_LEN );

	// fill out a copy for the delayed version of this function
	delayedfinfo = finfo;

	delayedfinfo.m_return_type	= GPGetTypeID< GPDelayedEvaluation< R > >();
	delayedfinfo.m_invoke_ptr	= reinterpret_cast< uintptr_t >( delayed_invoke_func );

	// we will be storing the original version of the delayed function after
	// the delayed function
	delayedfinfo.m_original_function_id = m_nFuncs + 1;

	// add it to the list of known function in the environment
	m_functions[ m_nFuncs++ ]	= delayedfinfo;
	m_functions[ m_nFuncs ]		= finfo;
	return m_nFuncs++;
}

#endif