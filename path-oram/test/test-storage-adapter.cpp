#include "definitions.h"
#include "storage-adapter.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace PathORAM;

class StorageAdapterTest : public ::testing::Test
{
	private:
	protected:
	vector<Edge> edges = {
		Edge{10, 1, 30}, // source
		Edge{10, 2, 30},
		Edge{10, 3, 30},

		Edge{1, 4, 20}, // dead end

		Edge{1, 5, 20},
		Edge{2, 5, 30},
		Edge{2, 6, 40},
		Edge{3, 6, 50},

		Edge{5, 6, 100},

		Edge{5, 150, 15},
		Edge{5, 8, 25},
		Edge{6, 8, 35},
		Edge{6, 9, 45},

		Edge{150, 11, 30}, // sink
		Edge{8, 11, 30},
		Edge{9, 11, 30}};

	long source = 10;
	long sink   = 11;

	long result = 75;
};

TEST_F(StorageAdapterTest, EmptyInput)
{
	ASSERT_EQ(3, 3);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
