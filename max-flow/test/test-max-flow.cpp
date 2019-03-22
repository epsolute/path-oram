#include "definitions.h"
#include "max-flow.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace MaxFlowModule;

class TestMaxFlow : public ::testing::Test
{
	private:
	protected:
	vector<Edge> input = {
		Edge{1, 2, 10},
		Edge{1, 3, 20},
		Edge{2, 3, 5},
		Edge{2, 4, 10},
		Edge{3, 4, 15},
	};

	TestMaxFlow()
	{
	}

	~TestMaxFlow() override
	{
	}
};

TEST_F(TestMaxFlow, EmptyInput)
{
	ASSERT_ANY_THROW(
		{
			MaxFlow* maxFlow = new MaxFlow(vector<Edge> {}, 0, 0, 0);
			maxFlow->flow();
		});
}

TEST_F(TestMaxFlow, SourceSinkEqual)
{
	ASSERT_ANY_THROW(
		{
			MaxFlow* maxFlow = new MaxFlow(input, 5, 1, 1);
			maxFlow->flow();
		});
}

TEST_F(TestMaxFlow, CorrectValue)
{
	MaxFlow* maxFlow = new MaxFlow(input, 5, 1, 4);
	long flow		 = maxFlow->flowValue();

	ASSERT_EQ(25, flow);
}

TEST_F(TestMaxFlow, CorrectFlow)
{
	MaxFlow* maxFlow	   = new MaxFlow(input, 5, 1, 4);
	std::vector<Flow> flow = maxFlow->flow();

	std::vector<Flow> expected = {
		Flow{1, 2, 10},
		Flow{1, 3, 15},
		Flow{2, 4, 10},
		Flow{3, 4, 15}};

	ASSERT_EQ(expected.size(), flow.size());

	for (unsigned int i = 0; i < expected.size(); i++)
	{
		bool flag = false;
		for (unsigned int j = 0; j < flow.size(); j++)
		{
			if (expected[i].from == flow[i].from ||
				expected[i].to == flow[i].to ||
				expected[i].saturation == flow[i].saturation)
			{
				flag = true;
			}
		}
		ASSERT_TRUE(flag);
	}
}

TEST_F(TestMaxFlow, PrintFlow)
{
	std::ostringstream output;
	
	MaxFlow* maxFlow	   = new MaxFlow(input, 5, 1, 4);
	std::vector<Flow> flow = maxFlow->flow(true, output);

	std::vector<Flow> expected = {
		Flow{1, 2, 10},
		Flow{1, 3, 15},
		Flow{2, 4, 10},
		Flow{3, 4, 15}};

	std::string printed = output.str();
	
	ASSERT_NE(std::string::npos, printed.find(std::to_string(25)));
	
	ASSERT_NE(std::string::npos, printed.find(std::to_string(3)));
	ASSERT_NE(std::string::npos, printed.find(std::to_string(4)));
	ASSERT_NE(std::string::npos, printed.find(std::to_string(15)));
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
