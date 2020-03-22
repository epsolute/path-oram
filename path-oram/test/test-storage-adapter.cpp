#include "definitions.h"
#include "storage-adapter.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace PathORAM;

class StorageAdapterTest : public ::testing::Test
{
	public:
	inline static const unsigned int CAPACITY   = 10;
	inline static const unsigned int BLOCK_SIZE = 32;

	protected:
	InMemoryStorageAdapter* adapter = new InMemoryStorageAdapter(CAPACITY, BLOCK_SIZE);

	~StorageAdapterTest() override
	{
		delete adapter;
	}
};

TEST_F(StorageAdapterTest, Initialization)
{
	SUCCEED();
}

TEST_F(StorageAdapterTest, ReadWriteNoCrash)
{
	EXPECT_NO_THROW({
		adapter->set(CAPACITY - 1, bytes());
		adapter->get(CAPACITY - 2);
	});
}

TEST_F(StorageAdapterTest, ReadEmpty)
{
	auto returned = adapter->get(CAPACITY - 2);
	ASSERT_EQ(BLOCK_SIZE, returned.size());
}

TEST_F(StorageAdapterTest, IdOutOfBounds)
{
	ASSERT_ANY_THROW(adapter->get(CAPACITY + 1));
	ASSERT_ANY_THROW(adapter->set(CAPACITY + 1, bytes()));
}

TEST_F(StorageAdapterTest, DataTooBig)
{
	ASSERT_ANY_THROW(adapter->set(CAPACITY - 1, bytes(BLOCK_SIZE + 1, 0x08)));
}

TEST_F(StorageAdapterTest, ReadWhatWasWritten)
{
	auto data = bytes{0xa8};

	adapter->set(CAPACITY - 1, data);
	auto returned = adapter->get(CAPACITY - 1);

	data.resize(BLOCK_SIZE, 0x00);

	ASSERT_EQ(data, returned);
}

TEST_F(StorageAdapterTest, Override)
{
	auto data = bytes{0xa8};
	data.resize(BLOCK_SIZE, 0x00);

	adapter->set(CAPACITY - 1, data);
	data[0] = 0x56;

	adapter->set(CAPACITY - 1, data);

	auto returned = adapter->get(CAPACITY - 1);

	ASSERT_EQ(data, returned);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
