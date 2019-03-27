#ifndef DEFINITIONS_INCLUDED
#define DEFINITIONS_INCLUDED

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace MaxFlowModule
{
	using namespace boost;

	typedef struct
	{
		long from;
		long to;
		long weight;
	} Edge;

	typedef struct
	{
		long identifier;
		long weight;
	} WeightedVertex;

	typedef struct
	{
		long from;
		long to;
		long saturation;
	} Flow;

	typedef struct
	{
		long flowValue;
		MaxFlowModule::Flow* flow;
		long flowSize;
		double alpha;
	} Solution;

	typedef adjacency_list_traits<vecS, vecS, directedS> Traits;
	typedef adjacency_list<vecS, vecS, directedS, property<vertex_name_t, std::string>, property<edge_capacity_t, long, property<edge_residual_capacity_t, long, property<edge_reverse_t, Traits::edge_descriptor>>>> Graph;
}

#endif
