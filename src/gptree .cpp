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
#include "gptree.h"
#include "gpfunctionlookup.h"

const int GPConstSubtreeIter::INVALID_INDEX = -1;

GPConstSubtreeIter::GPConstSubtreeIter( const GPTree* tree )
{
	m_flattened = tree->Flatten();
	m_count = tree->Count();
}

GPConstSubtreeIter::GPConstSubtreeIter( const GPTreeNode* subtree )
{
	m_flattened = GPTree::FlattenSubtree( subtree );
	m_count = GPTree::CountSubtree( subtree );
}

GPConstSubtreeIter::~GPConstSubtreeIter()
{
	delete m_flattened;
}

int GPConstSubtreeIter::Random( bool prefer_nonroot ) const
{
	const int offset = ( prefer_nonroot ? 1 : 0 );
	return	m_count == 0	? INVALID_INDEX :
			m_count == 1	? 0 
							: ( offset + rand() % ( m_count - offset ) );
}

int GPConstSubtreeIter::Random( const GPFunctionLookup& functions, GPTypeID return_type, bool prefer_nonroot ) const
{
	if ( m_count == 0 ) return INVALID_INDEX;

	const int offset = ( prefer_nonroot ? 1 : 0 );
	int selected_start_index = m_count == 1 ? 0 : ( offset + rand() % ( m_count - offset ) );

	int current_index = selected_start_index;
	do
	{
		const GPFunctionDesc& funcDesc = functions.GetFunctionByID( GetNode( current_index )->functionID );

		if ( funcDesc.m_return_type == return_type ) 
		{
			return current_index;
			break;
		}

		current_index = ( current_index + 1 ) % m_count;
		if ( prefer_nonroot && current_index == 0 ) current_index = ( current_index + 1 ) % m_count;
	}
	while( current_index != selected_start_index );

	return INVALID_INDEX;
}

void GPConstSubtreeIter::IgnoreNode( int index )
{
	assert( index < m_count && index >= 0 );
	--m_count;
	m_flattened[ index ] = m_flattened[ m_count ];
}

bool GPConstSubtreeIter::IgnoreSubtree( const GPTreeNode* node )
{
	for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
	{
		if ( node->parameters[ i ] )
		{
			IgnoreSubtree( node->parameters[ i ] );
		}
	}

	return IgnoreNode( node );
}

bool GPConstSubtreeIter::IgnoreNode( const GPTreeNode* node )
{
	for( int i = 0; i < m_count; ++i )
	{
		if ( GetNode( i ) == node )
		{
			IgnoreNode( i );
			return true;
		}
	}

	return false;
}


// ---------------------------------------------------------------------------
// GPReturnTypeIter
//	
// Limitations:
//
class GPReturnTypeIter
{
public:
	enum IterFlags
	{
		RANDOM_START	= 1,
		IGNORE_ROOT		= 2
	};

	GPReturnTypeIter( GPTree::FlattenedTreePtr flattened, int num_nodes, GPTypeID return_type_id, IterFlags flags );

private:

	int		IncrementAndWrap( int index ) const;
	bool	FindNextWithReturnType( int& index );

	GPTree::FlattenedTreePtr m_flattened_tree;
	int m_start_index, m_current_index, m_num_nodes;
	IterFlags m_flags;
};

GPReturnTypeIter::GPReturnTypeIter( GPTree::FlattenedTreePtr flattened, int num_nodes, GPTypeID return_type_id, IterFlags flags )
{
	m_flattened_tree = flattened;
	m_flags			= flags;
	m_num_nodes		= num_nodes;

	//
	// look for an index with our returntype to stop on.
	// UGLY: temporarily use m_current_index to know if we're looping.
	//
	m_start_index	= ( flags & RANDOM_START ) ? ( rand() % num_nodes ) : ( num_nodes - 1 );
	m_current_index	= m_start_index;
	do
	{
		// UGLY: shouldnt be calling member function in constructor really!
		m_start_index = IncrementAndWrap( m_start_index );
	}
	while( m_start_index != m_current_index && flattened[ m_start_index ]->functionID != return_type_id );

	// ensure we at least found this returntype in the tree
	assert( flattened[ m_start_index ]->functionID == return_type_id );

	// start
	m_current_index	= m_start_index;
}

int GPReturnTypeIter::IncrementAndWrap( int index ) const
{
	int ret = ( index + 1 ) % m_num_nodes;
	return ( m_flags & IGNORE_ROOT && ret == 0 ? 1 : ret );
}

GPTree::GPTree( int max_nodes )
{
	m_max_nodes	= max_nodes;
	m_count		= 0;
	m_root		= NULL;
}

GPTree::GPTree( const GPTree* other )
{
	m_max_nodes = other->m_max_nodes;
	m_count = other->m_count;

	m_root = Duplicate( other->m_root );
}

GPTree::~GPTree()
{
	DeleteSubtree( m_root );
}

const GPTreeNode*	GPTree::Root() const
{
	return m_root;
}


int					GPTree::Count()		const
{
	return m_count;
}

int					GPTree::MaxNodes()	const
{
	return m_max_nodes;
}

GPTree*				GPTree::Duplicate() const
{
	GPTree* new_tree = new GPTree( this );

	return new_tree;
}

GPTreeNode*	GPTree::Duplicate( const GPTreeNode * sourceTree )
{
	GPTreeNode * newNode = new GPTreeNode( sourceTree->functionID );

	for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
	{
		if ( sourceTree->parameters[ i ] )
		{
			newNode->parameters[ i ] = Duplicate( sourceTree->parameters[ i ] );
			newNode->parameters[ i ]->parent = newNode;
		}
		else
		{
			newNode->parameters[ i ] = NULL;
		}
	}

	return newNode;
}

GPTree::ConstFlattenedTreePtr		GPTree::Flatten()	const
{
	return const_cast< GPTree::ConstFlattenedTreePtr >( FlattenSubtree( m_root ) );
}

GPTree::ConstFlattenedTreePtr	GPTree::FlattenSubtree( const GPTreeNode* node )
{
	return const_cast< GPTree::ConstFlattenedTreePtr >( GPTree::FlattenSubtree( const_cast< GPTreeNode* >( node ) ) );
}

GPTree::FlattenedTreePtr		GPTree::FlattenSubtree( GPTreeNode* node )
{
	int num_nodes = GPTree::CountSubtree( node );
	FlattenedTreePtr out;
	out = new GPTreeNode*[ num_nodes ];
	GPTreeNode **working = new GPTreeNode*[ num_nodes ];

	int read_working_index	= 0;
	int write_working_index	= 1 % num_nodes; // if we only have 1 node, we dont run the while loop
	int write_out_index		= 0;

	// initialize the working list with the first function to process
	// copy to the out as well since the while loop wont get run if we have only one parameter
	out[ write_out_index ] = working[ 0 ] = node;
	while( write_working_index != read_working_index )
	{
		// move this function to the flat list
		out[ write_out_index ] = working[ read_working_index ];

		// write this functions parameters to the working list so we will deal with them
		for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
		{
			GPTreeNode * potential_parameter = out[ write_out_index ]->parameters[ i ];

			if ( potential_parameter )
			{
				working[ write_working_index ] = potential_parameter;
				write_working_index = ( write_working_index + 1 ) % num_nodes;
			}
		}

		read_working_index = ( read_working_index + 1 ) % num_nodes;
		++write_out_index;
	}

	delete working;
	return out;
}

// ---------------------------------------------------------------------------
// Replace
//		Given a source node (or subtree) within the tree, and a new subtree
//		this will replace the source with the target, if there is enough room.
//		It returns the 'spare' subtree which is now left hanging.
//
// Limitations:
//		Returned subtree can sometimes have an invalid parent!
//		Will not make room (Prune) the tree if the target cannot fit.
//
GPTreeNode*		GPTree::Replace( const GPTreeNode* source_node, GPTreeNode* new_subtree )
{
	int new_subtree_size = CountSubtree( new_subtree );

	if ( source_node == NULL )
	{
		// cant fit the subtree in
		if ( new_subtree_size > MaxNodes() ) return new_subtree;

		GPTreeNode * temp = m_root;
		m_root	= new_subtree;
		m_count	= new_subtree_size;

		return temp;
	}
	else
	{
		int existing_subtree_size = CountSubtree( source_node );

		// just ensure that existing subtree lies within our structure
		const GPTreeNode * parent;
		for( parent = source_node; parent != NULL && parent != m_root; parent = parent->parent );
		if ( parent == NULL ) return new_subtree;

		// ensure the new subtree can fit in the maxnodes for this tree
		if ( Count() - existing_subtree_size + new_subtree_size  > MaxNodes() ) return new_subtree;

		if ( source_node->parent )
		{
			new_subtree->parent = source_node->parent;

			for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
			{
				if ( source_node->parent->parameters[ i ] == source_node ) 
				{
					source_node->parent->parameters[ i ] = new_subtree;
					break;
				}
			}
		}
		else
		{
			// replacing the root
			m_root = new_subtree;
		}

		m_count = CountSubtree( m_root );

		return const_cast< GPTreeNode* >( source_node );
	}
}

GPTreeNode*		GPTree::Stitch( const GPFunctionLookup& functions, FlattenedTreePtr flattened, int num_flattened_nodes, int max_nodes )
{
	int parameters_index = 1;
	for( int i = 0; i < num_flattened_nodes; ++i )
	{
		if ( flattened[ i ]->functionID != GPFunctionLookup::NULLFUNC )
		{
			const GPFunctionDesc& function_desc = functions.GetFunctionByID( flattened[ i ]->functionID );

			for( int j = 0; j < function_desc.m_nparams; ++j )
			{
				flattened[ parameters_index ]->parent = flattened[ i ];
				flattened[ i ]->parameters[ j ] = flattened[ parameters_index++ ];
			}
		}
	}

	return flattened[ 0 ];
}

int			GPTree::CountSubtree( const GPTreeNode* node )
{
	int result = 1;
	for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
	{
		if ( node->parameters[ i ] )
		{
			result += CountSubtree( node->parameters[ i ] );
		}
	}

	return result;
}

void		GPTree::DeleteSubtree( GPTreeNode* subtree )
{
	for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
	{
		if ( subtree->parameters[ i ] ) DeleteSubtree( subtree->parameters[ i ] );
	}

	delete subtree;
}