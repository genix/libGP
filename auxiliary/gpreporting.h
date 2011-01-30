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

#ifndef GPREPORTING_H
#define GPREPORTING_H

#include <sstream>
#include "..\include\GPEnvironment.h"

class GPStats;

// save out given tree to specified filename (overwriting it) in a format that Dotty understands
void SaveGraphViz( const GPFunctionLookup& functions, const char* filename, const GPTree* tree );

// save out all the individuals from an environment to 0.dot ... n.dot files
// prefix is for the filenames. ie: prefix0.dot .. prefixn.dot
void SaveAllIndividualsGraphViz( const GPEnvironment& environment, const char* prefix );

// given tree returned as text format
void SerializeTree( const GPFunctionLookup& functions, const GPTree* tree, std::stringstream& out_serialized );

// output from a SerializeTree can be passed in here to regenerate the tree.
// LIMITATION: all of the names of the functions cannot have spaces in them!
bool DeserializeTree( const GPFunctionLookup& functions, GPTree*& tree, std::stringstream& in_serialized );

// WORK IN PROGRESS: function to save out a series from the stats tracker as html/jscript code
// that will visualize it using Google Charts.
// TODO: figure out a way that this can convert the stats stored type to string without having to know it
void HTMLGraphSeries( const char* filename, const GPStats* stats, const char* statname );


#endif