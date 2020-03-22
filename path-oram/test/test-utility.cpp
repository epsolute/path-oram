#include "definitions.h"
#include "utility.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace PathORAM;

class UtilityTest : public ::testing::Test
{
};

TEST_F(UtilityTest, DifferentValues)
{
	const auto n = 20uLL;
	for (int i = 0; i < 100; i++)
	{
		auto first  = getRandomBlock(n);
		auto second = getRandomBlock(n);

		ASSERT_NE(first, second);
	}
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
