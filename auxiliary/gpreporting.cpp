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

#include "GPReporting.h"
#include "GPStats.h"
#include <sstream>

using std::ofstream;

void SaveGraphViz( const GPFunctionLookup& functions, const char* filename, const GPTree* tree )
{
	GPConstSubtreeIter flattened( tree );

	ofstream fout( filename );

	fout << "digraph Individual {\n";
	fout << "size = \"6,9\";\n";
	fout << "node [ shape = record ];\n";

	// define all the nodes
	for( int i = 0; i < flattened.Count(); ++i )
	{
		const GPFunctionDesc& funcDesc = functions.GetFunctionByID( flattened.GetNode( i )->functionID );
		fout << i << " [ label = \"" << funcDesc.m_debug_name << "\"];\n ";
	}

	// output all the links
	for( int i = 0; i < tree->Count(); ++i )
	{
		for( int j = 0; j < GP_MAX_PARAMETERS; ++j )
		{
			const GPTreeNode* param = flattened.GetNode( i )->parameters[ j ];

			if ( param )
			{
				for( int k = 0; k < tree->Count(); ++k )
				{
					if ( flattened.GetNode( k ) == param )
					{
						fout << i << " -> " << k << ";\n";
						break;
					}
				}
			}
		}
	}

	fout << "}";
	fout.close();
}

void SaveAllIndividualsGraphViz( const GPEnvironment& environment, const char* prefix )
{
	for( int i = 0; i < environment.GetPopulationSize(); ++i )
	{
		char raw[64];
		std::stringstream fname( raw );

		fname << prefix << i << ".dot" << '\0';

		SaveGraphViz( environment, fname.str().c_str(), environment.GetIndividualByIndex( i ) );
	}
}

void SerializeTree( const GPFunctionLookup& functions, const GPTree* tree, std::stringstream& out_serialized )
{
	GPConstSubtreeIter flattened( tree );

	for( int i = 0; i < tree->Count(); ++i )
	{
		const GPTreeNode*		this_node		= flattened.GetNode( i );
		const GPFunctionDesc&	this_function	= functions.GetFunctionByID( this_node->functionID );

		out_serialized << i << " " << this_function.m_debug_name;

		for( int p = 0; p < GP_MAX_PARAMETERS; ++p )
		{
			if ( this_node->parameters[ p ] != NULL )
			{
				// not really optimal, but ill just search for the param
				int j = i + 1;
				for( ; j < tree->Count(); ++j )
				{
					if ( flattened.GetNode( j ) == this_node->parameters[ p ] )
					{
						out_serialized << " " << j;
						break;
					}
				}
				assert( j != tree->Count() );
			}
		}

		out_serialized << std::endl;
	}
}

// ---------------------------------------------------------------------------
// DeserializeTree
//		Given the functionset, and a stringstream to read from this will
//		deserialize a tree from its 'text' form. Returns true on success.
//
//	Limitations:
//		Function names cannot have spaces or it will give errors deserializing
//
bool DeserializeTree( const GPFunctionLookup& functions, GPTree*& tree, std::stringstream& in_serialized )
{
	struct IntermediateNode
	{
		// filled in as we read the nodes from the serialized buffer
		GPFuncID functionID;
		GPFuncID delayedID;

		// return types (as an easy reference for later calculations)
		GPTypeID originalReturnType;
		GPTypeID delayedReturnType;

		int params[ GP_MAX_PARAMETERS ];

		// calculated later (this is essentially which node will be the parent)
		int referencedByIndex;

		// set in the final phase when constructing the tree
		GPTreeNode * finalNode;
	};

	tree = NULL;
	typedef std::map< int, IntermediateNode > IntermediateNodes;
	IntermediateNodes nodes;

	in_serialized.seekg( 0, std::ios::beg );

	while( !in_serialized.eof() )
	{
		// read in one node
		std::string functionName;
		int this_node_index = -1;

		try
		{
			in_serialized >> this_node_index;
		}
		catch( const std::exception &e )
		{
			continue;
		}

		if ( nodes.find( this_node_index ) != nodes.end() || this_node_index == -1 )
		{
			continue;
		}

		in_serialized >> functionName;
		
		// grab the delayed version since we might need it. we can get the original function from the delayed info.
		const GPFuncID delayed_function_id = functions.GetFunctionIDByName( functionName.c_str(), true );
		if ( delayed_function_id == GPFunctionLookup::NULLFUNC ) 
		{
			return false;
		}
		const GPFunctionDesc&	delayed_function		= functions.GetFunctionByID( delayed_function_id );
		const GPFuncID			original_function_id	= delayed_function.m_original_function_id;

		IntermediateNode int_node;
		int_node.delayedID			= delayed_function_id;
		int_node.functionID			= original_function_id;
		int_node.finalNode			= NULL;
		int_node.referencedByIndex	= -1;
		int_node.originalReturnType	= functions.GetFunctionByID( original_function_id ).m_return_type;
		int_node.delayedReturnType	= delayed_function.m_return_type;

		for( int i = 0; i < delayed_function.m_nparams; ++i )
		{
			in_serialized >> int_node.params[ i ];
		}
		
		nodes[ this_node_index ] = int_node;
	}

	//
	// Create some placeholder nodes for the entire tree
	for( IntermediateNodes::iterator iter = nodes.begin(); iter != nodes.end(); ++iter )
	{
		IntermediateNode&		this_node		= iter->second;

		this_node.finalNode = new GPTreeNode(	this_node.functionID, 
												NULL,
												NULL,
												NULL
											);
	}

	//
	// for every node found, set its parent
	bool tree_is_intact = true;
	for( IntermediateNodes::iterator iter = nodes.begin(); iter != nodes.end() && tree_is_intact; ++iter )
	{
		IntermediateNode&		this_node		= iter->second;
		const GPFunctionDesc&	this_function	= functions.GetFunctionByID( this_node.functionID );

		for( int i = 0; i < this_function.m_nparams; ++i )
		{
			// make sure the referenced node exists!
			if ( nodes.find( this_node.params[ i ] ) == nodes.end() ) 
			{
				tree_is_intact = false;
				break;
			}

			IntermediateNode&	parameter_node		= nodes[ this_node.params[ i ] ];
			GPTypeID		param_type_required	= this_function.m_param_types[ i ];

			//
			// up until now we dont know if any node needs to use the delayed version of its function
			// since we're now linking the tree together, we can swap a node if its parent requires
			// it to be delayed.
			if ( parameter_node.originalReturnType != param_type_required )
			{
				assert( parameter_node.delayedReturnType	== param_type_required );

				parameter_node.finalNode->functionID = parameter_node.delayedID;
			}

			parameter_node.referencedByIndex = iter->first;
			parameter_node.finalNode->parent = this_node.finalNode;
			this_node.finalNode->parameters[ i ] = nodes[ this_node.params[ i ] ].finalNode;
		}
	}

	//
	// if we encountered an error setting the tree up, we need to clean up dangling ptrs
	if ( !tree_is_intact )
	{
		for( IntermediateNodes::iterator iter = nodes.begin(); iter != nodes.end(); ++iter )
		{
			IntermediateNode& this_node = iter->second;

			delete this_node.finalNode;
		}

		return false;
	}

	//
	// if all is done, find and return the parent node
	for( IntermediateNodes::iterator iter = nodes.begin(); iter != nodes.end(); ++iter )
	{
		IntermediateNode&		this_node		= iter->second;

		if ( this_node.referencedByIndex == -1 )
		{
			tree = new GPTree( GPTree::CountSubtree( this_node.finalNode ) );
			tree->Replace( NULL, this_node.finalNode );
			break;
		}
	}

	// SHOULD NEVER HIT THIS ASSERT?
	assert( tree != NULL );

	return true;
}


void HTMLGraphSeries( const char* filename, const GPStats* stats, const char* statname )
{
	// TODO:
	// at the moment this assumes GPFitness is the value - it should really work off any value
	//
	ofstream fout( filename );

	fout << "<html>\n";
	fout << "	<head>\n";
	fout << "		<title> Fitness </title>\n";
	fout << "		<script language=\"javascript\" src=\"http://www.google.com/jsapi\"></script>\n";
	fout << "	</head>\n";
	fout << "	<body>\n";
	fout << "		<div id=\"chart\"></div>\n";
	fout << "		<script type=\"text/javascript\">\n";
	fout << "  var queryString = '';\n";
	fout << "  var dataUrl = '';\n";
	fout << "	  function onLoadCallback() {\n";
	fout << "	if (dataUrl.length > 0) {\n";
	fout << "	  var query = new google.visualization.Query(dataUrl);\n";
	fout << "	 query.setQuery(queryString);\n";
	fout << "	  query.send(handleQueryResponse);\n";
	fout << "	} else {\n";
	fout << "	  var dataTable = new google.visualization.DataTable();\n";

	typedef GPStats::GPStatsValueList< GPFitness >::ValueList List;
	const List * valuelist;
	bool ret = stats->GetListValues< GPFitness >( GPS_BESTFITNESS, valuelist );
	// TODO: this return is false if no Tracking has been done of stats. maybe cater for this in the graph and 
	// let the caller know?
	assert( ret );

	int nValues = 0;
	std::ostringstream htmlcodedvalues;
	std::ostringstream commaseparatedvalues;
	for( List::const_iterator iter = valuelist->begin(); iter != valuelist->end(); ++iter )
	{
		htmlcodedvalues << "	  dataTable.setValue(" << nValues << ", 0, " << *iter << ");\n";
		commaseparatedvalues << *iter << ",";
		++nValues;
	}

	fout << "	  dataTable.addRows(" << nValues << ");\n";
	fout << "	  dataTable.addColumn('number');\n";
	fout << htmlcodedvalues.str();

	fout << "	  draw(dataTable);\n";
	fout << "	}\n";
	fout << "  }\n";
	fout << "  function draw(dataTable) {\n";
	fout << "	var vis = new google.visualization.ImageChart(document.getElementById('chart'));\n";
	fout << "	var options = {\n";
	fout << "	  chs: '1000x300',\n";
	fout << "	  cht: 'lc',\n";
	fout << "	  chco: '3072F3',\n";
	fout << "	  chdl: '" << statname << "',\n";
	fout << "	  chdlp: 'b',\n";
	fout << "	  chls: '2,4,1',\n";
	fout << "	  chtt: 'Fitness'\n";
	fout << "	};\n";
	fout << "	vis.draw(dataTable, options);\n";
	fout << "  }\n";
	fout << "  function handleQueryResponse(response) {\n";
	fout << "	if (response.isError()) {\n";
	fout << "	  alert('Error in query: ' + response.getMessage() + ' ' + response.getDetailedMessage());\n";
	fout << "	  return;\n";
	fout << "   }\n";
	fout << "   draw(response.getDataTable());\n";
	fout << "  }\n";
	fout << "  google.load(\"visualization\", \"1\", {packages:[\"imagechart\"]});\n";
	fout << "  google.setOnLoadCallback(onLoadCallback);\n";
	fout << "	</script>\n";
	fout << " </body>\n";
	fout << "</html>\n";

	fout.close();
}