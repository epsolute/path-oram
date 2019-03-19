#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace boost;

typedef struct
{
	long from;
	long to;
	long weight;
} Edge;

typedef struct
{
	long from;
	long to;
	long saturation;
} Flow;

typedef adjacency_list_traits<vecS, vecS, directedS> Traits;
typedef adjacency_list<vecS, vecS, directedS, property<vertex_name_t, std::string>, property<edge_capacity_t, long, property<edge_residual_capacity_t, long, property<edge_reverse_t, Traits::edge_descriptor>>>> Graph;

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
	MaxFlow(Edge edges[], long nVertices, long nEdges, long s, long t);
	~MaxFlow();

	long flowValue();
	std::vector<Flow> flow(bool print = false);
};

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
