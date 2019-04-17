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

		vector<Edge> constructEdges(double seedAlpha);
		double saturatedSource(vector<Flow> flow, vector<Edge> edges, double alpha);
		std::tuple<long, std::vector<Flow>> solveBasic();

		public:
		Solver(std::vector<Edge> edges, std::vector<WeightedVertex> vertices);
		std::tuple<long, std::vector<Flow>, double> solve(double maxAlpha, double precisionEpsilon);
		void amplify(long amplifier);

		friend class SolverTest_Initialization_Test;
		friend class SolverTest_ConstructEdges_Test;
		friend class SolverTest_SaturatedSource_Test;
		friend class SolverTest_Amplify_Test;
		friend class SolverTest_Solve_Test;
	};
}
