#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

typedef struct
{
	long source;
	long destination;
	long weight;
} Edge;

typedef adjacency_list_traits<vecS, vecS, directedS> Traits;
typedef adjacency_list<vecS, vecS, directedS, property<vertex_name_t, std::string>, property<edge_capacity_t, long, property<edge_residual_capacity_t, long, property<edge_reverse_t, Traits::edge_descriptor>>>> Graph;

class MaxFlow
{
	private:
	Graph G;
	Traits::vertex_descriptor source;
	Traits::vertex_descriptor sink;

	public:
	MaxFlow(Edge edges[], long source, long sink);
	~MaxFlow();

	long flow();
};
