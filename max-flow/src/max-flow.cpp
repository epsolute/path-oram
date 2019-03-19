#include "max-flow.hpp"

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/push_relabel_max_flow.hpp>
#include <boost/graph/read_dimacs.hpp>

using namespace boost;

MaxFlow::MaxFlow(Edge edges[], long nVertices, long nEdges, long s, long t)
{
	capacity									  = get(edge_capacity, G);
	property_map<Graph, edge_reverse_t>::type rev = get(edge_reverse, G);
	residual_capacity							  = get(edge_residual_capacity, G);

	std::ostringstream input;
	input << "c Input for max flow problem" << std::endl;
	input << "p max " << nVertices << " " << nEdges << std::endl;
	input << "n " << s << " s" << std::endl;
	input << "n " << t << " t" << std::endl;
	for (long i = 0; i < nEdges; i++)
	{
		input << "a " << edges[i].from << " " << edges[i].to << " " << edges[i].weight << std::endl;
	}
	std::string inputStr = input.str();
	std::istringstream inputStream(inputStr);

	read_dimacs_max_flow(G, capacity, rev, source, sink, inputStream);
}

MaxFlow::~MaxFlow()
{
}

long MaxFlow::flow()
{
	long flow = push_relabel_max_flow(G, source, sink);

	std::cout << "The total flow: " << flow << std::endl;
	std::cout << "The flow values:" << std::endl;
	graph_traits<Graph>::vertex_iterator u_iter, u_end;
	graph_traits<Graph>::out_edge_iterator ei, e_end;
	for (boost::tie(u_iter, u_end) = vertices(G); u_iter != u_end; ++u_iter)
	{
		for (boost::tie(ei, e_end) = out_edges(*u_iter, G); ei != e_end; ++ei)
		{
			if (capacity[*ei] > 0)
			{
				std::cout << (*u_iter) + 1 << " -> " << target(*ei, G) + 1 << " : " << (capacity[*ei] - residual_capacity[*ei]) << std::endl;
			}
		}
	}

	return flow;
}
