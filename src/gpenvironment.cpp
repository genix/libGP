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

#include "gpenvironment.h"



// returns function with given returntype, and which has at most maxPayload number of parameters
GPFuncID FindFunctionWithMaxPayload( const GPFunctionLookup& functions, const GPTypeID return_type, const int maxPayload )
{
	GPFuncID	start	= functions.GetRandomFuncWithReturnType( return_type ),
				current = start;

	if ( current == GPFunctionLookup::NULLFUNC ) 
	{
		return GPFunctionLookup::NULLFUNC;
	}

	do
	{
		const GPFunctionDesc& currentDesc = functions.GetFunctionByID( current );

		if ( currentDesc.m_nparams <= maxPayload ) return current;

		current = functions.GetNextFuncWithReturnType( return_type, current );
	}
	while( current != start );

	return GPFunctionLookup::NULLFUNC;
}

// ---------------------------------------------------------------------------
// CreateRandomTree
//		Creates a random subtree, and returns a pointer to the root.
//		Caller specifies the returntype for the subtree, and maximum number of
//		nodes which can be allocated. Additionally the number actually used
//		is returned through reference parameter.
//
//	Limitations
//		For any given return type, the algorithm assumes the GP will have a
//		function registered which returns that type, but has no parameters.
//		This is because if the algorithm finds itself in a position where it
//		has 1 node left to fill, it cant backtrack if a fill cannot be found.
//
GPTreeNode* CreateRandomTree(	const GPFunctionLookup& functions,
								const GPTypeID			return_type, 
								int&					nodes_used, 
								const int				max_nodes = 1 )
{
	// new flattened tree list - ready to hold the nodes!
	GPTree::FlattenedTreePtr flattened_tree = new GPTreeNode*[ max_nodes ];
	for( int i = 0; i < max_nodes; ++i )
	{
		flattened_tree[ i ] = NULL;
	}

	nodes_used = 1;
	flattened_tree[ 0 ] = new GPTreeNode( FindFunctionWithMaxPayload( functions, return_type, max_nodes - nodes_used ) );
	const GPFunctionDesc& first_function_desc = functions.GetFunctionByID( flattened_tree[ 0 ]->functionID );

	int process_function_idx	= 0;
	int parameter_write_idx		= 1;
	int reserved_nodes			= first_function_desc.m_nparams;
	while( process_function_idx < nodes_used )
	{
		if ( flattened_tree[ process_function_idx ]->functionID != GPFunctionLookup::NULLFUNC )
		{
			const GPFunctionDesc& function_desc = functions.GetFunctionByID( flattened_tree[ process_function_idx ]->functionID );
			if ( function_desc.m_nparams > 0 )
			{
				for( int j = 0; j < function_desc.m_nparams; ++j )
				{
					// since we're going to decide on this node now, we can remove this from the
					// reserved nodes and put it in the nodes_used pile
					--reserved_nodes;
					++nodes_used;

					int nodesRemaining = max_nodes - nodes_used - reserved_nodes;
					// select random function with returntype that fits within remaining nodes
					// TODO: if this function is unable to find a node to fit, its most likely because the return type
					// does not have a no-parameter function for it. report this sensibly to the user!
					GPFuncID parameterFuncID = FindFunctionWithMaxPayload( functions, function_desc.m_param_types[ j ], nodesRemaining );
					assert( parameterFuncID != GPFunctionLookup::NULLFUNC );
					// write it to the parameter write index, and increment it
					flattened_tree[ parameter_write_idx++ ] = new GPTreeNode( parameterFuncID );
					// add the selected function's payload to the reserved nodes
					const GPFunctionDesc& parameterFuncDesc = functions.GetFunctionByID( parameterFuncID );
					reserved_nodes += parameterFuncDesc.m_nparams;
				}
			}
		}

		++process_function_idx;
	}

	// now stitch this tree together
	GPTreeNode * newsubtree = GPTree::Stitch( functions, flattened_tree, parameter_write_idx, max_nodes );

	delete flattened_tree;

	return newsubtree;
}

// ---------------------------------------------------------------------------
// MutateTree:
//		Will select a non-root node of the given tree, and attempt to generate
//		a replacement subtree of any size (fitting within its max nodes)
//
// Limitations:
//		Subtree replacement is constrained to the same size. Maybe allow it to
//		use more nodes?
//
void MutateTree( const GPFunctionLookup& functions, GPTree* tree )
{
	GPConstSubtreeIter flattened( tree );

	// select a random node to mutate which is preferably not the root
	int mutateNode = flattened.Random( true );

	const GPTreeNode* oldSubtree = flattened.GetNode( mutateNode );
	int subtreeCount = GPTree::CountSubtree( oldSubtree );
	GPTypeID subtreeReturnType = functions.GetFunctionByID( flattened.GetNode( mutateNode )->functionID ).m_return_type;
	int subtreeNodes = 0;
	int availableNodes = tree->MaxNodes() - tree->Count() + subtreeCount;
	GPTreeNode* new_subtree = CreateRandomTree(	functions, subtreeReturnType, subtreeNodes, availableNodes );

	// if we were able to generate a subtree, then do the swap
	if ( new_subtree != NULL )
	{
		// try replace the subtree's - whichever subtree is returned is the spare one
		// and needs to be cleaned up
		GPTree::DeleteSubtree( tree->Replace( oldSubtree, new_subtree ) );
	}

}

// ---------------------------------------------------------------------------
// Prune:
//		Iterates over given tree finding nodes which have a subtree coming
//		off them that can be replaced with a single leaf node.
//		Caller specifies the number of nodes to attempt to remove.
//		Optionally a node can be specified which has to be preserved. This 
//		node and its direct parents will be guaranteed to still exist in the
//		pruned tree.
//
// Limitations:
//		* The algorithm fails if the node selected to be pruned cannot be
//		replaced with a single node (ie: there isnt a parameter-less function
//		for that returntype)
//		* The algorithm cannot reverse an operation, and does not calculate
//		whether the Prune can reach the desired target. So in the event
//		it cannot prune desired number of nodes, it prunes as many as it can.
//
int Prune(		const GPFunctionLookup& functions, 
				GPTree*					tree, 
				int						numToPrune, // or ability to prune down to x nodes? 
				const GPTreeNode*		preserveNode = NULL )
{
	// TODO
	// Improvement would be not not randomly prune the leftover nodes after
	// the main preserve tree is removed
	// but to walk up the preserve tree making note of immediate parameters
	// and only have those nodes to choose from
	//

	GPConstSubtreeIter flattenedIter( tree );

	// if we have a node to preserve, we will remove its subtree
	// and its direct parents from the iterator's selection list.
	if ( preserveNode )
	{
		flattenedIter.IgnoreSubtree( preserveNode );

		const GPTreeNode* parent = preserveNode->parent;
		while( parent )
		{
			flattenedIter.IgnoreNode( parent );
			parent = parent->parent;
		}
	}

	int numPruned = 0;
	while( numPruned < numToPrune && flattenedIter.Count() )
	{
		int randomNode = flattenedIter.Random( true );

		const GPTreeNode*		node			= flattenedIter.GetNode( randomNode );
		const GPFunctionDesc&	function_desc	= functions.GetFunctionByID( node->functionID );

		// remove this from the iterator now, since the following code will be deleting it
		// NOTE: this means the iterator is set for the following loop before this one finishes
		flattenedIter.IgnoreSubtree( node );

		// if this node has parameters, then we will bother pruning it
		bool canBePruned = false;
		for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
		{
			if ( node->parameters[ i ] )
			{
				canBePruned = true;
				break;
			}
		}

		if ( canBePruned )
		{
			int nodes_used = 0;
			GPTreeNode *replacement = CreateRandomTree(	functions, function_desc.m_return_type, nodes_used, 1 );
			assert( replacement );

			// do the prune with replacement
			GPTreeNode* unusedSubtree = tree->Replace( node, replacement );

			if ( unusedSubtree == node )
			{
				numPruned += GPTree::CountSubtree( unusedSubtree ) - 1;
			}

			delete	unusedSubtree;
		}

	}

	return numPruned;
}

int CalculatePotentialPrunes( const GPTreeNode* node )
{
	// add its subtree count
	int subtreeCount = GPTree::CountSubtree( node );

	// and every branch off the parents which is not the main line (-1 for replacement node)
	int potentialPrunes = 0;
	const GPTreeNode *subtree		= node;
	const GPTreeNode *subtreeParent = node->parent;
	while( subtreeParent )
	{
		for( int i = 0; i < GP_MAX_PARAMETERS; ++i )
		{
			if ( subtreeParent->parameters[ i ] && subtreeParent->parameters[ i ] != subtree )
			{
				potentialPrunes += GPTree::CountSubtree( subtreeParent->parameters[ i ] ) - 1;
			}
		}
		subtree			= subtreeParent;
		subtreeParent	= subtreeParent->parent;
	}

	return subtreeCount + potentialPrunes;
}

bool CrossOver( const GPFunctionLookup& functions, GPTree* sourceTree, GPTree* targetTree )
{
	assert( sourceTree != targetTree );

	GPConstSubtreeIter flattenedSource( sourceTree );

	const int			space_left_in_source	= sourceTree->MaxNodes() - sourceTree->Count();
	const int			space_left_in_target	= targetTree->MaxNodes() - targetTree->Count();
	const GPTreeNode*	selected_src_node		= NULL;
	const GPTreeNode*	selected_target_node	= NULL;
	int src_nodes_to_prune		= 0;
	int target_nodes_to_prune	= 0;

	while( flattenedSource.Count() && selected_target_node == NULL )
	{
		// select random source node
		const int			random_src_index	= flattenedSource.Random( true );
		const GPTreeNode *	source_node			= flattenedSource.GetNode( random_src_index );
		const int			src_potential_space	= CalculatePotentialPrunes( source_node ) + space_left_in_source;
		const int			src_subtree_count	= GPTree::CountSubtree( source_node );

		GPConstSubtreeIter flattened_target( targetTree );

		// while still nodes in target,
		while( flattened_target.Count() )
		{
			const GPFunctionDesc&	source_function_desc = functions.GetFunctionByID( source_node->functionID );

			int random_target_node = flattened_target.Random( functions, source_function_desc.m_return_type, true );
	
			if ( random_target_node != GPConstSubtreeIter::INVALID_INDEX )
			{
				const GPTreeNode *target_node			= flattened_target.GetNode( random_target_node );
				const int		target_potential_space	= CalculatePotentialPrunes( target_node ) + space_left_in_target;
				const int		target_subtree_count	= GPTree::CountSubtree( target_node );

				// can be swapped if subtree count of each fits within num_supported_replacement_nodes
				const bool src_fits_in_target	= src_subtree_count		<= target_potential_space;
				const bool targ_fits_in_src		= target_subtree_count	<= src_potential_space;
				if ( src_fits_in_target && targ_fits_in_src ) 
				{
					selected_src_node		= source_node;
					selected_target_node	= target_node;
					src_nodes_to_prune		= target_subtree_count - src_subtree_count - space_left_in_source;
					target_nodes_to_prune	= src_subtree_count - target_subtree_count - space_left_in_target;
					break;
				}

				flattened_target.IgnoreNode( random_target_node );
			}
			else
			{
				break;
			}
		}

		flattenedSource.IgnoreNode( random_src_index );
	}

	if ( selected_target_node != NULL )
	{
		if ( src_nodes_to_prune > 0 )
		{
			int n_pruned = Prune( functions, sourceTree, src_nodes_to_prune, selected_src_node );
			assert( n_pruned >= src_nodes_to_prune );
		}

		if ( target_nodes_to_prune > 0 )
		{
			int n_pruned = Prune( functions, targetTree, target_nodes_to_prune, selected_target_node );
			assert( n_pruned >= target_nodes_to_prune );
		}

		// TODO: Replace expects the first node to be within the 'source tree'
		// so we need to duplicate the subtrees we're swapping. ideally have a BranchSwap()
		// between the trees
		GPTreeNode* duplicate_src_subtree		= GPTree::Duplicate( selected_src_node );
		GPTreeNode* duplicate_target_subtree	= GPTree::Duplicate( selected_target_node );

		GPTreeNode* leftOverSourceSubtree = sourceTree->Replace( selected_src_node, duplicate_target_subtree );
		assert( leftOverSourceSubtree == selected_src_node );

		GPTreeNode* leftOverTargetSubtree = targetTree->Replace( selected_target_node, duplicate_src_subtree );
		assert( leftOverTargetSubtree == selected_target_node );

		delete leftOverSourceSubtree;
		delete leftOverTargetSubtree;
	}

	return selected_target_node != NULL;
}

GPEnvironment::GPEnvironment()
{
	m_population_size		= 0;
	m_population			= NULL;
	m_fitness_and_test_func	= NULL;
	m_fitness_func			= NULL;
	m_max_tree_size			= 10;
	m_return_type			= GP_INVALID_PARAMTYPE;
}

GPEnvironment::~GPEnvironment()
{
	for( int i = 0; i < m_population_size; ++i )
	{
		if ( m_population[ i ].m_tree ) delete m_population[ i ].m_tree;
	}
	if ( m_population ) delete m_population;
}

template<>
GPFitness GPEnvironment::EvaluateAndFitnessTest<void>( int index )
{
	typedef GPFitness(*FitnessFunc)( GPEnvironment&, const int );
	FitnessFunc custom_function = reinterpret_cast< FitnessFunc >( m_fitness_func );
	ExecuteTree< void >( *this, m_population[ index ].m_tree->Root() );
	return custom_function( *this, index );
}

void GPEnvironment::SetFitnessFunction( GPFitness(*fitnessFunc)( GPEnvironment&, const int ) )
{
	m_return_type = GPGetTypeID< void >();
	m_fitness_and_test_func = &GPEnvironment::EvaluateAndFitnessTest< void >;

	// TODO: assert that the void* can take the size of this pointer
	m_fitness_func = reinterpret_cast<void*>(fitnessFunc);
}

void GPEnvironment::OverrideIndividualFitness( int index, GPFitness fitness )
{
	m_population[ index ].m_current_fitness = fitness;
}

GPFitness GPEnvironment::EvaluateIndividual( int index )
{
	m_population[ index ].m_current_fitness = (*this.*m_fitness_and_test_func)( index );
	return m_population[ index ].m_current_fitness;
}

void GPEnvironment::EvaluateAll()
{
	for( int i = 0; i < m_population_size; ++i )
	{
		EvaluateIndividual( i );
	}
}

int	GPEnvironment::GetPopulationSize() const
{
	return m_population_size;
}

GPFitness	GPEnvironment::GetBestFitness() const
{
	GPFitness bestFit = -std::numeric_limits<double>::max();
	for( int i = 0; i < m_population_size; ++i )
	{
		if ( m_population[ i ].m_current_fitness > bestFit )
		{
			bestFit = m_population[ i ].m_current_fitness;
		}
	}

	return bestFit;
}

GPFitness GPEnvironment::GetIndividualFitness( int idx ) const
{
	return m_population[ idx ].m_current_fitness;
}

GPFitness	GPEnvironment::GetAverageFitness() const
{
	// TODO: what to do if any value still has -std::numeric_limits<double>::max() ??
	GPFitness total = 0;
	for( int i = 0; i < m_population_size; ++i )
	{
		total += m_population[ i ].m_current_fitness;
	}

	return total / m_population_size;
}

int			GPEnvironment::GetFittestIndividual() const
{
	int fittestIndex = 0;
	GPFitness bestFit = -std::numeric_limits<double>::max();
	for( int i = 0; i < m_population_size; ++i )
	{
		if ( m_population[ i ].m_current_fitness > bestFit )
		{
			bestFit = m_population[ i ].m_current_fitness;
			fittestIndex = i;
		}
	}
	return fittestIndex;
}

const GPTree*	GPEnvironment::GetIndividualByIndex( int idx ) const
{
	return m_population[ idx ].m_tree;
}

void GPEnvironment::SetIndividualReturnType( GPTypeID type )
{
	m_return_type = type;
	m_fitness_func = NULL;
}

void GPEnvironment::SetPopulationSize( int i )
{
	if ( m_population ) delete m_population;

	m_population_size = i;
	m_population = new Individual[ i ];

	for( int i = 0; i < m_population_size; ++i )
	{
		m_population[ i ].m_current_fitness = -std::numeric_limits<double>::max();
		m_population[ i ].m_tree = NULL;
	}
}

void GPEnvironment::SetMaxTreeSize( int i )
{
	m_max_tree_size = i;
}

bool GPEnvironment::OverrideIndividual( int idx, GPTree* replacement )
{
	assert( idx < GetPopulationSize() );

	//
	// ensure this replacement can fit our registered functions
	//
	GPConstSubtreeIter flattened( replacement );

	for( int i = 0; i < flattened.Count(); ++i )
	{
		const GPTreeNode* this_node = flattened.GetNode( i );

		// if we dont have such an id, then it doesnt fit
		if ( !FunctionIDExists( this_node->functionID ) ) return false;

		const GPFunctionDesc& this_function = GetFunctionByID( this_node->functionID );

		// check the parameter count & return types for the node
		for( int j = 0; j < GP_MAX_PARAMETERS; ++j )
		{
			bool hasParam				= this_node->parameters[ j ] != NULL;
			bool expectsParam			= this_function.m_nparams > j;

			if(  hasParam && !expectsParam || !hasParam && expectsParam ) return false;
			if( !hasParam && !expectsParam ) continue;

			bool paramFunctionExists	= FunctionIDExists( this_node->parameters[ j ]->functionID );

			if ( !paramFunctionExists ) return false;

			const GPFunctionDesc& paramFunction = GetFunctionByID( this_node->parameters[ j ]->functionID );

			GPFuncID hasType		= paramFunction.m_return_type;
			GPFuncID expectsType	= this_function.m_param_types[ j ];

			// TODO: functions are registered twice. delayed version first.
			// the deserialize doesnt match up the expected types, so we have function linking to
			// the delayed version of a parameter.
			//
			if ( hasType != expectsType ) return false;
		}
	}

	//
	// if we passed the checks, we'll just do a replace on the individual in question
	//
	delete m_population[ idx ].m_tree;
	m_population[ idx ].m_tree = replacement;
	m_population[ idx ].m_current_fitness = -std::numeric_limits<double>::max();

	return true;
}

void GPEnvironment::GenerateNewPopulation()
{
	for( int i = 0; i < m_population_size; ++i )
	{
		if ( m_population[ i ].m_tree ) delete m_population[ i ].m_tree;

		int nodes_used;
		m_population[ i ].m_current_fitness = -std::numeric_limits<double>::max();
		m_population[ i ].m_tree = new GPTree( m_max_tree_size );

		// todo: ensure this tree actually gets created and replace doesnt fail
		m_population[ i ].m_tree->Replace( NULL, CreateRandomTree( *this, m_return_type, nodes_used, m_max_tree_size ) );
	}
}

void GPEnvironment::MutateAndCrossover()
{
	enum BREED_ACTION 
	{
		GP_KEEP,	// keep this individual as-is
		GP_TWOWAY,	// do a 2-way crossover (both individuals automatically move to new pool)
		GP_ONEWAY,	// do a 1-way crossover (only the target individual moves into the new gene pool)
		GP_COPYOF,	// copy specified tree (overwriting the current one entirely)
		GP_NEW		// generate an entirely new individual for this slot
	};

	enum BREED_INDEX_RELATIVITY
	{
		GP_RELATIVE,	// the 'partner' index for ActionInfo is relative to the current tree
		GP_ABSOLUTE,	// the 'partner' index for ActionInfo is based on the original ranked list
		GP_NOPARTNER
	};

	struct ActionInfo
	{
		BREED_ACTION	m_action;
		bool			m_mutate;

		// the index of the partner tree for our action (if the action requires one)
		// this is either relative to the current tree, or an absolute index into ranked list
		BREED_INDEX_RELATIVITY	m_partner_index_relativity;
		int						m_partner_index; 

		ActionInfo( BREED_ACTION action, bool mutate, BREED_INDEX_RELATIVITY relativity, int partner_index )
			: m_action( action ), m_mutate( mutate ), m_partner_index_relativity( relativity ), m_partner_index( partner_index )
		{
		}
	};

	// TODO:
	// at the moment effects are cumulative, so if a tree is mutated
	// and later a tree wants to crossover with it, the crossover will happen with mutated version.
	// ALSO:
	// - only new trees will change the 'root' node, which means after some success is had
	//   there will be no experimentation with the root node D:
	// - any not specified by actions should maybe default to the last entry
	// - if the successful nodes are count 1, crossovers and such are fail
	ActionInfo actions[] = 
	{
		ActionInfo( GP_KEEP,	false,	GP_NOPARTNER,	0 ),
		ActionInfo( GP_KEEP,	false,	GP_NOPARTNER,	0 ),
		ActionInfo( GP_ONEWAY,	true,	GP_ABSOLUTE,	0 ),
		ActionInfo( GP_ONEWAY,	true,	GP_ABSOLUTE,	0 ),
		ActionInfo( GP_ONEWAY,	true,	GP_ABSOLUTE,	1 ),
		ActionInfo( GP_ONEWAY,	true,	GP_ABSOLUTE,	1 ),
		ActionInfo( GP_TWOWAY,	true,	GP_ABSOLUTE,	2 ),
		ActionInfo( GP_TWOWAY,	true,	GP_ABSOLUTE,	1 ),
		ActionInfo( GP_COPYOF,	true,	GP_ABSOLUTE,	0 ),
		ActionInfo( GP_COPYOF,	true,	GP_ABSOLUTE,	1 ),
		ActionInfo( GP_COPYOF,	true,	GP_ABSOLUTE,	0 ),
		ActionInfo( GP_COPYOF,	true,	GP_ABSOLUTE,	1 ),
		ActionInfo( GP_COPYOF,	true,	GP_ABSOLUTE,	2 ),
		ActionInfo( GP_COPYOF,	true,	GP_ABSOLUTE,	3 ),
		ActionInfo( GP_NEW,		false,	GP_NOPARTNER,	0 ),
		ActionInfo( GP_NEW,		false,	GP_NOPARTNER,	0 ),
		ActionInfo( GP_NEW,		false,	GP_NOPARTNER,	0 ),
		ActionInfo( GP_COPYOF,	true,	GP_ABSOLUTE,	0 ),
	};

	int n_actions = sizeof( actions ) / sizeof( ActionInfo );

	//
	// rank all the individuals by their fitness levels
	//
	int *ranked_by_fitness	= new int[ m_population_size ];
	int *original_rankings	= new int[ m_population_size ];

	// todo: better sorting algorithm needed!
	int write_index = 0;
	for( int i = 0; i < m_population_size; ++i )
	{
		ranked_by_fitness[ write_index ] = i;
		int swap_index = write_index;
		while( swap_index > 0 && m_population[ ranked_by_fitness[ swap_index ] ].m_current_fitness > m_population[ ranked_by_fitness[ swap_index - 1 ] ].m_current_fitness )
		{
			int t = ranked_by_fitness[ swap_index ];
			ranked_by_fitness[ swap_index ] = ranked_by_fitness[ swap_index - 1 ];
			ranked_by_fitness[ swap_index - 1 ] = t;
			--swap_index;
		}
		++write_index;
	}

	memcpy( original_rankings, ranked_by_fitness, sizeof( int ) * m_population_size );

	//
	// apply the changes
	//
	int nLeftToProcess = m_population_size;
	for( int i = 0; i < m_population_size; ++i )
	{
		//
		// for each item in ranked fitness, apply the necessary change
		// then remove it, and if it had a partner, remove that from the ranked fitness list
		//
		int partner_index;
		int current_index	= ranked_by_fitness[ 0 ];
		int action_index	= std::min( i, n_actions - 1 );

		const ActionInfo&	current_action = actions[ action_index ];

		Individual&	current_individual = m_population[ current_index ];

		switch( current_action.m_partner_index_relativity )
		{
		case GP_RELATIVE :
			{
				assert( current_action.m_partner_index != 0 );
				partner_index = ranked_by_fitness[ current_action.m_partner_index ];
				break;
			}
		case GP_ABSOLUTE :
			{
				assert( current_action.m_partner_index >= 0 && current_action.m_partner_index < m_population_size );
				partner_index = original_rankings[ current_action.m_partner_index ];
				break;
			}
		default:
			partner_index = 0;
		}

		assert( current_index < m_population_size && current_index >= 0 );
		assert( partner_index < m_population_size && partner_index >= 0 );
		switch( current_action.m_action )
		{
		case GP_KEEP :
			{
				// no action needed
				break;
			}
		case GP_TWOWAY :
			{
				// no real way to handle failed crossover other than to make note it occurred
				bool success = CrossOver( *this, current_individual.m_tree, m_population[ partner_index ].m_tree );
				if ( !success )
				{
					m_stats.IncrementCounter( GPS_FAILEDXOVERS );
				}
				m_stats.IncrementCounter( GPS_TOTALXOVERS );
				break;
			}
		case GP_ONEWAY :
			{
				// TODO:
				// since our crossover will actually swap between two trees, an easy 'way out' is
				// to duplicate the target so the original doesnt get modified during the operation
				GPTree* target_copy = m_population[ partner_index ].m_tree->Duplicate();
				bool success = CrossOver( *this, current_individual.m_tree, target_copy );
				if ( !success )
				{
					m_stats.IncrementCounter( GPS_FAILEDXOVERS );
				}
				m_stats.IncrementCounter( GPS_TOTALXOVERS );
				delete target_copy;
				break;
			}
		case GP_NEW :
			{
				int nodes_used;
				delete current_individual.m_tree;
				current_individual.m_tree = new GPTree( m_max_tree_size );
				current_individual.m_tree->Replace( NULL, CreateRandomTree( *this, m_return_type, nodes_used, m_max_tree_size ) );
				current_individual.m_current_fitness = -std::numeric_limits<double>::max();
				assert( current_individual.m_tree->Count() > 0 );
				break;
			}
		case GP_COPYOF:
			{
				delete current_individual.m_tree;
				current_individual.m_tree = m_population[ partner_index ].m_tree->Duplicate();
				current_individual.m_current_fitness = m_population[ partner_index ].m_current_fitness;
				break;
			}
		};

		if ( current_action.m_mutate )
		{
			MutateTree( *this, current_individual.m_tree );
			current_individual.m_current_fitness = -std::numeric_limits<double>::max();
		}

		nLeftToProcess--;
		for( int j = 0; j < nLeftToProcess; ++j )
		{
			ranked_by_fitness[ j ] = ranked_by_fitness[ j + 1 ];
		}
	}

	delete[] ranked_by_fitness;
	delete[] original_rankings;
}

void GPEnvironment::TrackStats()
{
	GPFitness bestFitness	= GetBestFitness();
	GPFitness avgFitness	= GetAverageFitness();

	m_stats.PushListValue< GPFitness >( GPS_BESTFITNESS, bestFitness );
	m_stats.PushListValue< GPFitness >( GPS_AVGFITNESS, avgFitness );
}
