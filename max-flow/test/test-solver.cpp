#include "definitions.h"
#include "solver.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace MaxFlowModule;

namespace MaxFlowModule
{
	class SolverTest : public ::testing::Test
	{
		private:
		protected:
		vector<Edge> edges = {
			Edge{1, 4, 20}, // dead end

			Edge{1, 5, 20},
			Edge{2, 5, 30},
			Edge{2, 6, 40},
			Edge{3, 6, 50},

			Edge{5, 6, 100},

			Edge{5, 7, 15},
			Edge{5, 8, 25},
			Edge{6, 8, 35},
			Edge{6, 9, 45}};

		vector<WeightedVertex> vertices = {
			WeightedVertex{1, -verticesWeight},
			WeightedVertex{2, -verticesWeight},
			WeightedVertex{3, -verticesWeight},

			WeightedVertex{4, 0},
			WeightedVertex{5, 0},
			WeightedVertex{6, 0},

			WeightedVertex{7, verticesWeight},
			WeightedVertex{8, verticesWeight},
			WeightedVertex{9, verticesWeight},
		};

		static const long verticesWeight = 30;

		long source = 10;
		long sink   = 11;
	};

	TEST_F(SolverTest, EmptyInput)
	{
		ASSERT_ANY_THROW(
			{
				new Solver(vector<Edge>{}, vector<WeightedVertex>{});
			});
	}

	TEST_F(SolverTest, Initialization)
	{
		auto* solver = new Solver(edges, vertices);

		ASSERT_EQ(3, solver->addedSourceEdges.size());
		ASSERT_EQ(3, solver->addedSinkEdges.size());
		ASSERT_EQ(edges.size(), solver->originalEdges.size());

		for (auto edge : solver->addedSourceEdges)
		{
			ASSERT_EQ(source, edge.from);
		}

		for (auto edge : solver->addedSinkEdges)
		{
			ASSERT_EQ(sink, edge.to);
		}

		for (auto edge : solver->originalEdges)
		{
			ASSERT_NE(source, edge.to);
			ASSERT_NE(source, edge.from);
			ASSERT_NE(sink, edge.to);
			ASSERT_NE(sink, edge.from);
		}

		ASSERT_EQ(source, solver->source);
		ASSERT_EQ(sink, solver->sink);
	}

	TEST_F(SolverTest, ConstructEdges)
	{
		auto* solver = new Solver(edges, vertices);

		double alpha			= 2.0;
		auto edges				= solver->constructEdges(alpha);
		vector<Edge> fromSource = {};

		auto source = this->source;
		std::copy_if(edges.begin(), edges.end(), std::back_inserter(fromSource), [source](Edge e) { return e.from == source; });

		ASSERT_EQ(3, fromSource.size());

		for (auto edge : fromSource)
		{
			ASSERT_EQ(source, edge.from);
			ASSERT_EQ(alpha * verticesWeight, edge.weight);
		}
	}

	TEST_F(SolverTest, SaturatedSource)
	{
		auto* solver = new Solver(edges, vertices);

		vector<Flow> flow = {
			Flow{source, 1, (long)std::round(verticesWeight * 1.0)},
			Flow{source, 2, (long)std::round(verticesWeight * 0.7)},
			Flow{source, 3, (long)std::round(verticesWeight * 0.5)},

			Flow{2, 5, verticesWeight},
			Flow{2, 6, verticesWeight},
			Flow{8, sink, verticesWeight}};

		auto saturation = solver->saturatedSource(flow, solver->addedSourceEdges, 1.0);

		ASSERT_NEAR((1.0 + 0.7 + 0.5) / 3, saturation, 0.0001);
	}

	TEST_F(SolverTest, Amplify)
	{
		auto* solver  = new Solver(edges, vertices);
		auto original = solver->constructEdges(1.0);

		solver->amplify(5);

		auto amplified = solver->constructEdges(1.0);

		for (auto pair : zip(original, amplified))
		{
			auto [edge, amp] = asStdTuple(pair);

			ASSERT_EQ(edge.weight * 5, amp.weight);
		}
	}

	TEST_F(SolverTest, Solve)
	{
		auto amplifier = 1000;
		auto expected  = 62;

		auto* solver = new Solver(edges, vertices);
		solver->amplify(amplifier);
		auto [value, flow, alpha] = solver->solve(1.0, 0.01);

		ASSERT_NEAR(expected * amplifier, value, amplifier);
		ASSERT_NE(0, flow.size());
		ASSERT_GT(1, alpha);
		ASSERT_LT(0, alpha);
	}
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
