#include "definitions.h"

#include <tuple>

namespace MaxFlowModule
{
	class Solver
	{
		private:
		
		long source;
		long sink;
		Edge edges[];
		
		public:
		
		Solver(std::vector<Edge> edges, std::vector<WeightedVertex>);
		std::tuple<double, std::vector<Flow>> solve(double seedAlpha, double precisionEpsilon);
	};
}
