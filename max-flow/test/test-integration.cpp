#include "definitions.h"
#include "max-flow.hpp"
#include "solver.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace MaxFlowModule;

Solution maxFlow(int, MaxFlowModule::Edge*, int, MaxFlowModule::WeightedVertex*, double, double);

namespace MaxFlowModule
{
	class IntegrationTest : public ::testing::Test
	{
		private:
		protected:
		// here define the values accessible inside the test methods
	};
	
	TEST_F(IntegrationTest, ExternC)
	{
		vector<Edge> edges = {
			Edge{1, 5, 20}, // dead end

			Edge{1, 5, 20},
			Edge{2, 5, 30},
			Edge{2, 6, 40},
			Edge{3, 6, 50},

			Edge{5, 6, 100},

			Edge{5, 7, 15},
			Edge{5, 8, 25},
			Edge{6, 8, 35},
			Edge{6, 9, 45}};

		long verticesWeight = 30;

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

		auto result = maxFlow(edges.size(), &edges[0], vertices.size(), &vertices[0], 1.0, 0.01);
		
		ASSERT_NEAR(75, result.flowValue, 0.01);
	}

	TEST_F(IntegrationTest, BasicMaxFlow)
	{
		vector<Edge> edges = {
			Edge{10, 1, 30}, // source
			Edge{10, 2, 30},
			Edge{10, 3, 30},

			Edge{1, 5, 20}, // dead end

			Edge{1, 5, 20},
			Edge{2, 5, 30},
			Edge{2, 6, 40},
			Edge{3, 6, 50},

			Edge{5, 6, 100},

			Edge{5, 7, 15},
			Edge{5, 8, 25},
			Edge{6, 8, 35},
			Edge{6, 9, 45},

			Edge{7, 11, 30}, // sink
			Edge{8, 11, 30},
			Edge{9, 11, 30}};

		long nVertices = 11;
		long source	= 10;
		long sink	  = 11;

		long result = 75;

		MaxFlow* maxFlow = new MaxFlow(edges, nVertices, source, sink);
		ASSERT_EQ(result, maxFlow->flowValue());
	}

	TEST_F(IntegrationTest, BasicSolver)
	{
		vector<Edge> edges = {
			Edge{1, 5, 20}, // dead end

			Edge{1, 5, 20},
			Edge{2, 5, 30},
			Edge{2, 6, 40},
			Edge{3, 6, 50},

			Edge{5, 6, 100},

			Edge{5, 7, 15},
			Edge{5, 8, 25},
			Edge{6, 8, 35},
			Edge{6, 9, 45}};

		long verticesWeight = 30;

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

		auto* solver			  = new Solver(edges, vertices);
		auto [value, flow, alpha] = solver->solve(1.0, 0.01);

		ASSERT_NEAR(75, value, 0.01);
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
