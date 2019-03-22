#include "definitions.h"

#include <iostream>

namespace MaxFlowModule
{
	class MaxFlow
	{
		private:
		Graph G;
		Traits::vertex_descriptor source;
		Traits::vertex_descriptor sink;

		property_map<Graph, edge_capacity_t>::type capacity;
		property_map<Graph, edge_residual_capacity_t>::type residual_capacity;

		long result = -1;

		public:
		MaxFlow(std::vector<Edge> edges, long nVertices, long s, long t);

		long flowValue();
		std::vector<Flow> flow(bool print = false, std::ostream& out = std::cout);
	};
}

/**
 * DIMACS file format:
 * 
 * c COMMENTS
 * p max VERTICES EDGES
 * n SOURCE s
 * n SINK t // sink
 * a FROM TO WEIGHT
 * ...
 * a FROM TO WEIGHT
 */
