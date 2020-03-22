#include "definitions.h"
#include "position-map-adapter.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace PathORAM;

class PositionMapAdapterTest : public ::testing::Test
{
	public:
	inline static const unsigned int CAPACITY = 10;

	protected:
	InMemoryPositionMapAdapeter* adapter = new InMemoryPositionMapAdapeter(CAPACITY);

	~PositionMapAdapterTest() override
	{
		delete adapter;
	}
};

TEST_F(PositionMapAdapterTest, Initialization)
{
	SUCCEED();
}

TEST_F(PositionMapAdapterTest, ReadWriteNoCrash)
{
	EXPECT_NO_THROW({
		adapter->set(CAPACITY - 1, 56uLL);
		adapter->get(CAPACITY - 2);
	});
}

TEST_F(PositionMapAdapterTest, BlockOutOfBounds)
{
	ASSERT_ANY_THROW(adapter->get(CAPACITY + 1));
	ASSERT_ANY_THROW(adapter->set(CAPACITY + 1, 56uLL));
}

TEST_F(PositionMapAdapterTest, ReadWhatWasWritten)
{
	auto leaf = 56uLL;

	adapter->set(CAPACITY - 1, leaf);
	auto returned = adapter->get(CAPACITY - 1);

	ASSERT_EQ(leaf, returned);
}

TEST_F(PositionMapAdapterTest, Override)
{
	auto old = 56uLL, _new = 25uLL;

	adapter->set(CAPACITY - 1, old);
	adapter->set(CAPACITY - 1, _new);

	auto returned = adapter->get(CAPACITY - 1);

	ASSERT_EQ(_new, returned);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
