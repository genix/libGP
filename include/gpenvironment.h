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

#ifndef GPENVIRONMENT_H
#define GPENVIRONMENT_H

#include "gpdefines.h"
#include "gpstats.h"
#include "gpfunctionLookup.h"

// some names for stats tracking
#define GPS_BESTFITNESS		"BestFitness"
#define GPS_AVGFITNESS		"AvgFitness"
#define GPS_FAILEDXOVERS	"FailedCrossovers"
#define GPS_TOTALXOVERS		"TotalCrossovers"

// ---------------------------------------------------------------------------
// GPEnvironment
//
// This plays host to the population of individuals being trained. Necessarily
// it also provides a place to register all the functions available as nodes
// in the individuals. 
//
// Requiring at a minimum only a fitness function of specific signature, and
// a host of registered functions it provides the mechanisms to generate,
// breed, and mutate toward an individual best suited to maximimise said
// fitness function.
//
// TODO:
//	- allow custom mutation and crossover functions
//
class GPEnvironment : public GPFunctionLookup
{
	typedef struct Individual
	{
		GPTree*		m_tree;
		GPFitness	m_current_fitness;
	};

	//
	// signature for a function which will execute an individual, 
	// grade its output by calling the registered fitnes sfunction and
	// return said fitness.
	typedef GPFitness(GPEnvironment::*FitnessAndTestFunctionPtr)( int );

public:
	GPEnvironment();
	~GPEnvironment();

	// let the environment know what type the individuals are expected to return
	// used for not only constructing a population, but also its what the fitness
	// functions is expected to accept.
	void SetIndividualReturnType( GPTypeID );

	// maximum number of nodes in any individual (size of each genetic program)
	void SetMaxTreeSize( int );

	// number of individuals in this population (set this before calling GenerateNewPopulation)
	void SetPopulationSize( int );

	// makes a new population of completely random individuals
	void GenerateNewPopulation();

	// applies mutation and crossover to the existing population. this function assumes
	// that every individual has already been tested and a fitness value has been stored.
	void MutateAndCrossover();

	// optional tracking of statistics for each generation of individuals. call this function
	// once per loop before the MutateAndCrossover() modifies the individuals.
	void TrackStats();

	// number of individuals in the population
	int	GetPopulationSize() const;

	// specify the fitness function to be used to test the output of each individual.
	// the fitness function needs to be global or static at this time.
	// example signature:
	//		GPFitness MeasureResult( GPEnvironment&, const int individual_index, const R& individuals_result )
	template< class R >
		void SetFitnessFunction( GPFitness(*fitnessFunc)( GPEnvironment&, const int, const R& ) );
	void SetFitnessFunction( GPFitness(*fitnessFunc)( GPEnvironment&, const int ) );

	// this is only valid after an individual has been Evaluate()'d
	GPFitness		GetBestFitness() const;

	// this is only valid after an individual has been Evaluate()'d
	GPFitness		GetAverageFitness() const;

	// this is only valid after an individual has been Evaluate()'d
	GPFitness		GetIndividualFitness( int idx ) const;

	// returns the index of the fittest individual
	// this is only valid after an individual has been Evaluate()'d
	int				GetFittestIndividual() const;

	// returns the GPTree for the fittest individual
	const GPTree*	GetIndividualByIndex( int idx ) const;

	void		EvaluateAll();
	GPFitness	EvaluateIndividual( int index );

	void		OverrideIndividualFitness( int index, GPFitness fitness );

	template< class R >
		R ExecuteIndividual( int index );

	const GPStats* GetStats() const { return &m_stats; }

	// NOTE: the tree provided will be owned by GPEnvironment (cleaned up)
	bool			OverrideIndividual( int idx, GPTree* );

private:
	template< class R >
		GPFitness EvaluateAndFitnessTest( int index );

	Individual					*m_population;
	FitnessAndTestFunctionPtr	m_fitness_and_test_func;

	// note: by storing the fitness function in a void*, it has to be global
	// or on some compilers more complex function pointers wont fit.
	void*			m_fitness_func;

	int				m_population_size;
	int				m_max_tree_size;
	GPTypeID		m_return_type;
	GPStats			m_stats;
};

template< class R >
void GPEnvironment::SetFitnessFunction( GPFitness(*fitnessFunc)( GPEnvironment&, const int, const R& ) )
{
	m_return_type = GPGetTypeID< R >();
	m_fitness_and_test_func = &GPEnvironment::EvaluateAndFitnessTest< R >;

	// TODO: assert that the void* can take the size of this pointer
	m_fitness_func = reinterpret_cast<void*>(fitnessFunc);
}

// We need both a general version of EvaluateAndFitness test for the case when
// the fitness function is expecting some kind of parameter
// as well as the case where the GP is is returning void.
template< class R >
GPFitness GPEnvironment::EvaluateAndFitnessTest( int index )
{
	typedef GPFitness(*FitnessFunc)( GPEnvironment&, const int, const R& );
	FitnessFunc custom_function = reinterpret_cast< FitnessFunc >( m_fitness_func );
	return custom_function( *this, index, ExecuteTree< R >( *this, m_population[ index ].m_tree->Root() ) );
}

template<>
GPFitness GPEnvironment::EvaluateAndFitnessTest<void>( int index );

template< class R >
R GPEnvironment::ExecuteIndividual( int index )
{
	// if this assert fires, the caller is asking for a type other than what the current
	// population are expected to be returning.
	assert( GPGetTypeID< R >() == m_return_type );
	return ExecuteTree< R >( *this, m_population[ index ].m_tree->Root() );
}


#endif