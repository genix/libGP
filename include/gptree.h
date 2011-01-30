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

#ifndef GPTREE_H
#define GPTREE_H

// ---------------------------------------------------------------------------
// GPTreeNode
//
// For each node in the GP, all we need to know is which function ID this
// node uses, and of course the tree links to other GPTreeNodes.
//
struct GPTreeNode
{
	// TODO: the parameters are fixed to 3. be nice to tie this in to GP_MAX_PARAMETERS
	GPTreeNode( GPFuncID function, GPTreeNode* p1 = NULL, GPTreeNode* p2 = NULL, GPTreeNode* p3 = NULL )
	{
		functionID = function;
		parameters[0] = p1;
		parameters[1] = p2;
		parameters[2] = p3;

		if ( p1 )
		{
			p1->parent = this;
		}
		if ( p2 )
		{
			p2->parent = this;
		}
		if ( p3 )
		{
			p3->parent = this;
		}

		parent = NULL;
	}

	// need to know the function we will call for this node
	GPFuncID functionID;

	// for each parameter this function might take, we need
	// to know the function which will provide it
	GPTreeNode *parameters[ GP_MAX_PARAMETERS ];

	// easy traversal of tree - store the parent
	GPTreeNode *parent;
};


// ---------------------------------------------------------------------------
// GPTree
//
// Implementation for representing a "GP Program" which has a number of 
// GPTreeNodes, and various functions for building, replacing and duplicating
// this GPTree.
//
// The responsibilities end at just representing the tree and enabling simple
// operations on it. Mutation/Crossover algorithms kept separate.
//	
// Limitations:
//
class GPTree
{
	friend class GPConstSubtreeIter;

public:
	typedef GPTreeNode** FlattenedTreePtr;
	typedef const GPTreeNode** ConstFlattenedTreePtr;

	GPTree( int max_nodes );
	GPTree( const GPTree* other );
	~GPTree();

	const GPTreeNode*	Root()		const;
	int					Count()		const;
	int					MaxNodes()	const;
	GPTree*				Duplicate() const;

	//
	// given a node already in this tree, it will be replaced by given node for subtree
	// NO RETURN TYPE CHECKS ARE DONE! Just a subtree swap
	// The function will return the source_node if the replace was successful, else
	// the new_subtree node if the replace could not occur (typically max_nodes for tree is exceeded)
	// either way, the returned pointer should be cleaned up (call DeleteSubtree)
	//
	GPTreeNode*			Replace( const GPTreeNode* source_node, GPTreeNode* new_subtree );


	// ---------------------------------------------------------------------------
	// Below are operations that are not specific to a particular tree
	// But never-the-less are operations that tree's should support. Thus
	// they are kept static.
	// ---------------------------------------------------------------------------

	//
	// Stitches a subtree together from a FlattenedTreePtr of the same format as returned by Flatten()
	// This will fix the parent/child relationships. Note the nodes in the FlattenedTreePtr are not duplicated!
	//
	static GPTreeNode*	Stitch( const GPFunctionLookup& functions, FlattenedTreePtr flattened, int num_flattened_nodes, int max_nodes );
	static void			DeleteSubtree( GPTreeNode* subtree );
	static int			CountSubtree( const GPTreeNode* node );
	//
	// duplicate a subtree
	//
	static GPTreeNode*	Duplicate( const GPTreeNode * sourceTree );

private:
	// 
	// flattens this tree into an array of GPTreeNode*'s which are stored breadth first
	// thus a tree of Add( Sub( Const1(), Const2() ), Sub( Const3(), Const4() ) ) would be returned in the order
	// Add, Sub, Sub, Const1, Const2, Const3, Const4
	//
	ConstFlattenedTreePtr			Flatten()	const;

	static ConstFlattenedTreePtr	FlattenSubtree( const GPTreeNode* node );
	static FlattenedTreePtr			FlattenSubtree( GPTreeNode* node );

	int	m_max_nodes;
	int m_count;

	GPTreeNode* m_root;
};

// ---------------------------------------------------------------------------
// GPConstSubtreeIter
//	This provides a way of iterating over a GPTree. If IgnoreNode is not used
//	then the iteration is breadth-first.
//
//	NOTE: this iterator is not safe across tree modifications! if an iterator
//	is being used, and the tree is modified in any way then the iterator
//	should be considered invalid!
//	
// Limitations:
//	 See note above.
//
// TODO:
// - be able to assert if a tree has been modified, and the iterator is invalid.
//
class GPConstSubtreeIter
{
	typedef const GPTreeNode** ConstFlattenedTreePtr;

public:
	GPConstSubtreeIter( const GPTree* tree );
	GPConstSubtreeIter( const GPTreeNode* subtree );
	~GPConstSubtreeIter();

	int					Count() const					{ return m_count; }
	const GPTreeNode*	GetNode( int index ) const		{ return m_flattened[ index ]; }

	// if prefer_nonroot if true, the function will return a non-root node
	// if such a node exists. if only one node is available (the root), then it is considered.
	int					Random( bool prefer_nonroot ) const;
	int					Random( const GPFunctionLookup& functions, GPTypeID return_type, bool prefer_nonroot ) const;

	// use this function to 'remove' nodes from the iterator
	// note: after using this, nodes will no longer be in breadth-first
	// order. order is not guaranteed to be anything particular.
	void				IgnoreNode( int index );
	// false is returned if the node given is found in the subtree
	bool				IgnoreNode( const GPTreeNode* node );
	bool				IgnoreSubtree( const GPTreeNode* node );

	const static int	INVALID_INDEX;
private:
	ConstFlattenedTreePtr m_flattened;
	int m_count;
};


#endif