#include "solver.hpp"

namespace MaxFlowModule
{
	Solver::Solver(std::vector<Edge> edges, std::vector<WeightedVertex>)
	{
		}

	std::tuple<double, std::vector<Flow>> Solver::solve(double seedAlpha, double precisionEpsilon)
	{
		return std::make_tuple(0.1, std::vector<Flow>{});
	}
}
