#include "definitions.h"

#include <tuple>

using namespace std;

namespace MaxFlowModule
{
	class Solver
	{
		private:
		
		long source;
		long sink;
		vector<Edge> originalEdges;
		vector<Edge> addedSourceEdges;
		vector<Edge> addedSinkEdges;
		long nVertices;
		
		vector<Edge> constructEdges(double seedAlpha);
		double saturatedSource(vector<Flow> flow, vector<Edge> edges, double alpha);
		
		public:
		
		Solver(std::vector<Edge> edges, std::vector<WeightedVertex> vertices);
		std::tuple<double, std::vector<Flow>> solve(double maxAlpha, double precisionEpsilon);
	};
}
