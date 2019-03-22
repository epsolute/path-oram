#include "max-flow.hpp"

#include <iostream>

using namespace std;
using namespace MaxFlowModule;

int main(int argc, char **argv)
{
	cout << "Simple tiny max flow computation" << endl;

	vector<Edge> edges = {
		Edge{1, 2, 10},
		Edge{1, 3, 20},
		Edge{2, 3, 5},
		Edge{2, 4, 10},
		Edge{3, 4, 15},
	};

	MaxFlow *mf = new MaxFlow(edges, 5, 1, 4);
	mf->flow(true);

	exit(0);
}
