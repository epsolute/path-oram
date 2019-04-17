#include "definitions.h"
#include "max-flow.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace MaxFlowModule;

class MaxFlowTest : public ::testing::Test
{
	private:
	protected:
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

	long source	= 10;
	long sink	  = 11;

	long result = 75;
};

TEST_F(MaxFlowTest, EmptyInput)
{
	ASSERT_ANY_THROW(
		{
			MaxFlow* maxFlow = new MaxFlow(vector<Edge>{}, 0, 0);
			maxFlow->flow();
		});
}

TEST_F(MaxFlowTest, SourceSinkEqual)
{
	ASSERT_ANY_THROW(
		{
			MaxFlow* maxFlow = new MaxFlow(edges, source, source);
			maxFlow->flow();
		});
}

TEST_F(MaxFlowTest, CorrectValue)
{
	MaxFlow* maxFlow = new MaxFlow(edges, source, sink);
	long flow		 = maxFlow->flowValue();

	ASSERT_EQ(result, flow);
}

TEST_F(MaxFlowTest, ConseravationConstraints)
{
	MaxFlow* maxFlow   = new MaxFlow(edges, source, sink);
	vector<Flow> flow  = maxFlow->flow();
	set<long> vertices = {};

	for (auto edge : edges)
	{
		vertices.insert(edge.from);
		vertices.insert(edge.to);
	}

	for (auto vertex : vertices)
	{
		long inOut = 0;
		for (auto edge : flow)
		{
			if (edge.from == vertex)
			{
				inOut += edge.saturation;
			}
			else if (edge.to == vertex)
			{
				inOut -= edge.saturation;
			}
		}

		if (vertex == source)
		{
			ASSERT_EQ(result, inOut);
		}
		else if (vertex == sink)
		{
			ASSERT_EQ(-result, inOut);
		}
		else
		{
			ASSERT_EQ(0, inOut);
		}
	}
}

TEST_F(MaxFlowTest, PrintFlow)
{
	std::ostringstream output;

	MaxFlow* maxFlow	   = new MaxFlow(edges, source, sink);
	std::vector<Flow> flow = maxFlow->flow(true, output);

	std::string printed = output.str();

	ASSERT_FALSE(printed.empty());
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
