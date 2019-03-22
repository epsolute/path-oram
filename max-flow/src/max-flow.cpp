#include "max-flow.hpp"

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/push_relabel_max_flow.hpp>
#include <boost/graph/read_dimacs.hpp>

namespace MaxFlowModule
{
	using namespace boost;

	MaxFlow::MaxFlow(std::vector<Edge> edges, long nVertices, long s, long t)
	{
		if (nVertices <= 0 || edges.size() == 0 || s == t)
		{
			throw "Malformed parameters";
		}

		capacity									  = get(edge_capacity, G);
		property_map<Graph, edge_reverse_t>::type rev = get(edge_reverse, G);
		residual_capacity							  = get(edge_residual_capacity, G);

		std::ostringstream input;
		input << "c Input for max flow problem" << std::endl;
		input << "p max " << nVertices << " " << edges.size() << std::endl;
		input << "n " << s << " s" << std::endl;
		input << "n " << t << " t" << std::endl;
		for (auto const& edge : edges)
		{
			input << "a " << edge.from << " " << edge.to << " " << edge.weight << std::endl;
		}
		std::string inputStr = input.str();
		std::istringstream inputStream(inputStr);

		read_dimacs_max_flow(G, capacity, rev, source, sink, inputStream);
	}

	std::vector<Flow> MaxFlow::flow(bool print, std::ostream& out)
	{
		std::vector<Flow> flows = {};

		if (result < 0)
		{
			flowValue();
		}

		if (print)
		{
			out << "The total flow: " << result << std::endl;
			out << "The flow values:" << std::endl;
		}

		graph_traits<Graph>::vertex_iterator u_iter, u_end;
		graph_traits<Graph>::out_edge_iterator ei, e_end;
		for (boost::tie(u_iter, u_end) = vertices(G); u_iter != u_end; ++u_iter)
		{
			for (boost::tie(ei, e_end) = out_edges(*u_iter, G); ei != e_end; ++ei)
			{
				if (capacity[*ei] > 0 && capacity[*ei] - residual_capacity[*ei] > 0)
				{
					Flow instance = Flow{(long)((*u_iter) + 1), (long)(target(*ei, G) + 1), capacity[*ei] - residual_capacity[*ei]};
					flows.push_back(instance);

					if (print)
					{
						out << instance.from << " -> " << instance.to << " : " << instance.saturation << std::endl;
					}
				}
			}
		}

		return flows;
	}

	long MaxFlow::flowValue()
	{
		result = push_relabel_max_flow(G, source, sink);
		return result;
	}
}
