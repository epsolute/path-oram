#include "definitions.h"
#include "gtest/gtest_prod.h"

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

		public:
		Solver(std::vector<Edge> edges, std::vector<WeightedVertex> vertices);
		std::tuple<long, std::vector<Flow>, double> solve(double maxAlpha, double precisionEpsilon);
		void amplify(long amplifier);

		FRIEND_TEST(SolverTest, Initialization);
		FRIEND_TEST(SolverTest, ConstructEdges);
		FRIEND_TEST(SolverTest, SaturatedSource);
		FRIEND_TEST(SolverTest, Amplify);
		FRIEND_TEST(SolverTest, Solve);
	};
}
