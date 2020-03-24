#include "definitions.h"
#include "utility.hpp"

#include "gtest/gtest.h"

using namespace std;

namespace PathORAM
{
	class UtilityTest : public ::testing::Test
	{
	};

	TEST_F(UtilityTest, NoSeed)
	{
		const auto n = 20uLL;
		for (ulong i = 0; i < 100; i++)
		{
			auto first  = getRandomBlock(n);
			auto second = getRandomBlock(n);

			ASSERT_NE(first, second);
		}
	}

	TEST_F(UtilityTest, SameSeed)
	{
		const auto n = 20uLL;
		int seed	 = 0x13;

		seedRandom(seed);

		auto first = getRandomBlock(n);

		seedRandom(seed);

		auto second = getRandomBlock(n);

		ASSERT_EQ(first, second);
	}
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
