#include "definitions.h"
#include "storage-adapter.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"

using namespace std;

namespace PathORAM
{
	class StorageAdapterTest : public ::testing::Test
	{
		public:
		inline static const ulong CAPACITY   = 10;
		inline static const ulong BLOCK_SIZE = 32;

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
			adapter->set(CAPACITY - 1, {5uLL, bytes()});
			adapter->get(CAPACITY - 2);
		});
	}

	TEST_F(StorageAdapterTest, ReadEmpty)
	{
		auto [id, data] = adapter->get(CAPACITY - 2);
		ASSERT_EQ(BLOCK_SIZE - sizeof(ulong), data.size());
	}

	TEST_F(StorageAdapterTest, IdOutOfBounds)
	{
		ASSERT_ANY_THROW(adapter->get(CAPACITY + 1));
		ASSERT_ANY_THROW(adapter->set(CAPACITY + 1, {5uLL, bytes()}));
	}

	TEST_F(StorageAdapterTest, DataTooBig)
	{
		ASSERT_ANY_THROW(adapter->set(CAPACITY - 1, {5uLL, bytes(BLOCK_SIZE + 1, 0x08)}));
	}

	TEST_F(StorageAdapterTest, ReadWhatWasWritten)
	{
		auto data = bytes{0xa8};
		auto id   = 5uLL;

		adapter->set(CAPACITY - 1, {id, data});
		auto [returnedId, returnedData] = adapter->get(CAPACITY - 1);

		data.resize(BLOCK_SIZE - sizeof(ulong), 0x00);

		ASSERT_EQ(id, returnedId);
		ASSERT_EQ(data, returnedData);
	}

	TEST_F(StorageAdapterTest, Override)
	{
		auto id   = 5;
		auto data = bytes{0xa8};
		data.resize(BLOCK_SIZE - sizeof(ulong), 0x00);

		adapter->set(CAPACITY - 1, {id, data});
		data[0] = 0x56;
		id		= 6;

		adapter->set(CAPACITY - 1, {id, data});

		auto [returnedId, returnedData] = adapter->get(CAPACITY - 1);

		ASSERT_EQ(id, returnedId);
		ASSERT_EQ(data, returnedData);
	}

	TEST_F(StorageAdapterTest, InitializeToEmpty)
	{
		for (ulong i = 0; i < CAPACITY; i++)
		{
			auto expected = bytes();
			expected.resize(BLOCK_SIZE - sizeof(ulong), 0x00);
			ASSERT_EQ(ULONG_MAX, adapter->get(i).first);
			ASSERT_EQ(expected, adapter->get(i).second);
		}
	}
}

int main(int argc, char** argv)
{
	PathORAM::seedRandom(0x13);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
