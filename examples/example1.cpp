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
#include "gpenvironment.h"
#include "gpreporting.h"

/*
 * This is the simplest example of using libGP. Its goals are to show:
 * - registration of functions to be used as nodes in a GP
 * - setup of the initial environment
 * - simple iteration of the population through crossover/mutation
 * - how to export an individual to a visualisable format
 *
 * It outputs using the auxiliary support to save for viewing with Dotty.
 *
 * The problem presented:
 * Create a program consisting of either Add, Multiply, 2 and 3 which will
 * return a value as close to kExpectedReturnValue as possible.
 *
 * There are no conditionals in this example.
 */

// this is what we're going to train our GP's to return
const int kExpectedReturnValue = 25;

// ---------------------------------------------------------------------------
// Firstly the functions we're going to be using for nodes

int Add( int a, int b )
{
	return a + b;
}

int Mul( int a, int b )
{
	return a * b;
}

int Two()
{
	return 2;
}

int Three()
{
	return 3;
}

// ---------------------------------------------------------------------------
// Next we'll need to have a fitness function with which the GPEnvironment can
// optimise the individuals.
// The fitness function has a standard signature. Its parameters are: 
// GPEnvironment	- the environment housing the individual being tested
// int				- the index of the individual being tested
// const int&		- this is the const& for the return type of the individual
//
GPFitness Fitness( GPEnvironment& environment, int individual_index, const int& individual_returned_value )
{
	// so for now we'll just look at what the individual has calculated, and
	// return how far off it is from an expected number. the larger the distance
	// the worse the score.

	return abs( kExpectedReturnValue - individual_returned_value ) * -1;
}

// ---------------------------------------------------------------------------
// Next we'll just to the entire test case in the main function as there is 
// not much to it.

int main(int argc, char* argv[])
{
	srand(time(NULL));

	// All the individuals will be housed in this environment.
	GPEnvironment environment;

	// Next we'll need to register the functions it can use to build each
	// individual
	environment.RegisterFunction( "Add",	Add );
	environment.RegisterFunction( "Mul",	Mul );
	environment.RegisterFunction( "Two",	Two );
	environment.RegisterFunction( "Three",	Three );

	// let it know a fitness function with which to test the GP's
	// for each iteration. The fitness function has a standard signature
	environment.SetFitnessFunction( Fitness );

	// so all that is left is to let the environment know how big 
	// we'll let the trees become, and how many individuals we want
	// in our population
	const int kMaxTreeSize		= 15;
	const int kPopulationSize	= 10;

	environment.SetMaxTreeSize( kMaxTreeSize );
	environment.SetPopulationSize( kPopulationSize );

	// so lets create a brand new population
	// totally random GP's using the above instruction set
	environment.GenerateNewPopulation();

	// evaluate the new population to get their fitness values
	environment.EvaluateAll();

	// now we'll iterate until we reach our target fitness level
	// (which for now we'll just require a 'perfect' answer)
	int iterations = 1;
	while( environment.GetBestFitness() < 0 )
	{
		// do the mutations and crossovers to generate the next generation
		environment.MutateAndCrossover();

		environment.EvaluateAll();

		++iterations;
	}

	// output a few statistics regarding our most successful individual
	std::cout << "The fittest individual had a score of " << environment.GetBestFitness() << std::endl;
	std::cout << "When executed, it returns result " << environment.ExecuteIndividual<int>( environment.GetFittestIndividual() ) << std::endl;
	std::cout << "The expected result is " << kExpectedReturnValue << std::endl;
	std::cout << "It took " << iterations << " iteration(s) to achieve the answer" << std::endl;

	// save out to fittest.dot file for viewing
	SaveGraphViz( environment, "fittest.dot", environment.GetIndividualByIndex( environment.GetFittestIndividual() ) );
	std::cout << "The fittest individual was saved to fittest.dot for viewing with Dotty graph viewer" << std::endl;

	return 0;
}

