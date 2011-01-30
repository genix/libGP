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


#include "..\include\gpdefines.h"
#include "..\include\gpenvironment.h"
#include "..\auxiliary\gpreporting.h"

/*
 * This is a slightly more advanced example of using libGP.
 * It intends to demonstrate:
 * - use of conditionals in GP structures
 * - use of varied return types
 * - a 'manual' way of controlling population evolution
 *
 * The problem is:
 * Each GP is a dog. It begins at 0,0 and has to find a stick at x,y.
 * The GP is required to move the dog to the stick.
 *
 */

// Some ugly globals. Perhaps if I make another example, Ill try show
// how test suites can be set up which dont use them.
int dog_x, dog_y;
int stick_x, stick_y;

// ---------------------------------------------------------------------------
// Firstly the functions we're going to be using for nodes

// ACTIONS

void DoNothing()
{
}

void MoveUp()
{
	--dog_y;
}

void MoveDown()
{
	++dog_y;
}

void MoveLeft()
{
	--dog_x;
}

void MoveRight()
{
	++dog_x;
}

// TESTS

bool StickIsUp()
{
	return stick_y < dog_y;
}

bool StickIsDown()
{
	return stick_y > dog_y;
}

bool StickIsLeft()
{
	return stick_x < dog_x;
}

bool StickIsRight()
{
	return stick_x > dog_x;
}

// CONDITIONALS

// Note the use of GPDelayedEvaluation<>. It binds to functions which return 
// the type of its template. However the value for that parameter is not
// immediately available to the function. Instead if the function wishes to
// see what value that parameter returns, the GPDelayedEvaluation<> should
// have its Evaluate() called.
void If( const bool condition, const GPDelayedEvaluation< void > true_action, const GPDelayedEvaluation< void > false_action )
{
	if ( condition )
	{
		true_action.Evaluate();
	}
	else
	{
		false_action.Evaluate();
	}
}

// ---------------------------------------------------------------------------
// Next the fitness function. Explained in example1.cpp
// NOTE that we dont expect our dog GP's to return anything. So by omitting
// the 3rd parameter, the environment will assume it to be 'void'.
//
GPFitness Fitness( GPEnvironment& environment, int individual_index )
{
	// simply penalize the dog for being far from the stick
	const int x_diff = stick_x - dog_x;
	const int y_diff = stick_y - dog_y;
	return ( x_diff * x_diff + y_diff * y_diff ) * -1;
}

// ---------------------------------------------------------------------------
// Setup each individuals test case
// This is an example of how one can perform the executions and fitness tests
// 'manually' without the framework automatically assigning fitness.
GPFitness EvaluateIndividual( GPEnvironment& environment, int individual_index )
{
	dog_x	= 0;
	dog_y	= 0;

	stick_x	= rand() % 21 - 10;
	stick_y	= rand() % 21 - 10;

	const int kTurns = 30;
	for( int i = 0; i < kTurns; ++i )
	{
		// each dog will get a kTurns in which to try retrieve the stick
		// and we just want to execute the individual - dont call the fitness
		// function yet!
		environment.ExecuteIndividual< void >( individual_index );
	}

	return Fitness( environment, individual_index );
}

// ---------------------------------------------------------------------------
// Just loop through all the individuals
void EvaluateAll( GPEnvironment& environment )
{
	int n_individuals = environment.GetPopulationSize();
	for( int i = 0; i < n_individuals; ++i )
	{
		// for each dog, we'll run the test a number of times
		// where each time a stick will be thrown, and the dog
		// will need to fetch it.
		// its fitness will be graded as an average of these attempts.
		GPFitness individual_fitness = 0;
		const int kThrows = 10;
		for( int j = 0; j < kThrows; ++j )
		{
			individual_fitness += EvaluateIndividual( environment, i );
		}

		individual_fitness = individual_fitness / kThrows;

		// after putting the dog through its paces, we will manually call the fitness
		// function to see how well it did. we can then override the fitness value
		// which the framework has for it.
		// NOTE: the framework needs to have either automatically called the Fitness
		// or otherwise had it set with an OverrideIndividualFitness() call
		// because it requires the fitness value to perform crossover and mutations.
		environment.OverrideIndividualFitness( i, individual_fitness );
	}
}

// ---------------------------------------------------------------------------
// The main test

int main(int argc, char* argv[])
{
	srand(time(NULL));

	// All the individuals will be housed in this environment.
	GPEnvironment environment;

	// Next we'll need to register the functions it can use to build each
	// individual
	environment.RegisterFunction( "DoNothing",		DoNothing );
	environment.RegisterFunction( "MoveUp",			MoveUp );
	environment.RegisterFunction( "MoveDown",		MoveDown );
	environment.RegisterFunction( "MoveLeft",		MoveLeft );
	environment.RegisterFunction( "MoveRight",		MoveRight );
	environment.RegisterFunction( "StickIsUp",		StickIsUp );
	environment.RegisterFunction( "StickIsDown",	StickIsDown );
	environment.RegisterFunction( "StickIsLeft",	StickIsLeft );
	environment.RegisterFunction( "StickIsRight",	StickIsRight );
	environment.RegisterFunction( "If",				If );

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
	// NOTE: we're going to call our own function here! not the one
	// on the framework.
	EvaluateAll( environment );

	// now we'll iterate until we reach our target fitness level
	// (which for now we'll just require a 'perfect' answer)
	int iterations = 1;
	while( environment.GetBestFitness() < 0 )
	{
		// do the mutations and crossovers to generate the next generation
		environment.MutateAndCrossover();

		EvaluateAll( environment );

		++iterations;
	}

	// output a few statistics regarding our most successful individual
	std::cout << "The fittest individual had a score of " << environment.GetBestFitness() << std::endl;
	std::cout << "It took " << iterations << " iteration(s) to achieve the answer" << std::endl;

	// save out to fittest.dot file for viewing
	SaveGraphViz( environment, "fittest.dot", environment.GetIndividualByIndex( environment.GetFittestIndividual() ) );
	std::cout << "The fittest individual was saved to fittest.dot for viewing with Dotty graph viewer" << std::endl;

	return 0;
}

