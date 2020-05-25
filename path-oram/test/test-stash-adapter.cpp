#include "definitions.h"
#include "stash-adapter.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"

using namespace std;

namespace PathORAM
{
	class StashAdapterTest : public ::testing::Test
	{
		public:
		inline static const number CAPACITY = 10;

		protected:
		unique_ptr<InMemoryStashAdapter> adapter = make_unique<InMemoryStashAdapter>(CAPACITY);
	};

	TEST_F(StashAdapterTest, Initialization)
	{
		SUCCEED();
	}

	TEST_F(StashAdapterTest, ReadGetEraseNoCrash)
	{
		EXPECT_NO_THROW({
			adapter->add(5uLL, bytes());
			vector<block> got;
			adapter->getAll(got);
			adapter->remove(5uLL);
		});
	}

	TEST_F(StashAdapterTest, LoadStore)
	{
		const auto blockSize = 64;
		const auto filename	 = "stash.bin";
		const auto expected	 = fromText("hello", blockSize);

		auto stash = make_unique<InMemoryStashAdapter>(CAPACITY);
		stash->add(5, expected);
		stash->storeToFile(filename);
		stash.reset();

		stash = make_unique<InMemoryStashAdapter>(CAPACITY);
		stash->loadFromFile(filename, blockSize);
		bytes read;
		stash->get(5, read);
		EXPECT_EQ(expected, read);

		remove(filename);
	}

	TEST_F(StashAdapterTest, LoadStoreFileError)
	{
		auto stash = new InMemoryStashAdapter(CAPACITY);
		ASSERT_ANY_THROW(stash->storeToFile("/error/path/should/not/exist"));
		ASSERT_ANY_THROW(stash->loadFromFile("/error/path/should/not/exist", 0));
		delete stash;
	}

	TEST_F(StashAdapterTest, GetAllShuffle)
	{
		for (number i = 0; i < CAPACITY; i++)
		{
			adapter->add(i, bytes());
		}

		vector<block> first;
		adapter->getAll(first);

		vector<block> second;
		adapter->getAll(second);

		EXPECT_EQ(first.size(), second.size());
		EXPECT_NE(first, second);
	}

	TEST_F(StashAdapterTest, OverflowAdd)
	{
		for (number i = 0uLL; i < CAPACITY; i++)
		{
			adapter->add(i, bytes());
		}
		ASSERT_ANY_THROW(adapter->add(CAPACITY + 1, bytes()));
		adapter->remove(0uLL);
		ASSERT_NO_THROW(adapter->add(CAPACITY + 1, bytes()));
		ASSERT_NO_THROW(adapter->add(CAPACITY + 1, bytes())); // duplicate key should not be inserted
	}

	TEST_F(StashAdapterTest, OverflowUpdate)
	{
		for (number i = 0uLL; i < CAPACITY; i++)
		{
			adapter->update(i, bytes());
		}
		ASSERT_ANY_THROW(adapter->update(CAPACITY + 1, bytes()));
		adapter->remove(0uLL);
		ASSERT_NO_THROW(adapter->update(CAPACITY + 1, bytes()));
		ASSERT_NO_THROW(adapter->update(CAPACITY + 1, bytes())); // duplicate key should not be inserted
	}

	TEST_F(StashAdapterTest, ReadWhatWasWritten)
	{
		auto block = CAPACITY - 1;
		auto data  = bytes{0x25};

		adapter->add(block, data);
		bytes returned;
		adapter->get(block, returned);

		ASSERT_EQ(data, returned);
	}

	TEST_F(StashAdapterTest, Override)
	{
		auto block = CAPACITY - 1;
		auto old = bytes{0x25}, _new = bytes{0x56};

		adapter->add(block, old);
		adapter->update(block, _new);

		bytes returned;
		adapter->get(block, returned);

		vector<pair<number, bytes>> got;
		adapter->getAll(got);
		ASSERT_EQ(1, got.size());
		ASSERT_EQ(_new, returned);
	}

	TEST_F(StashAdapterTest, NoOverride)
	{
		auto block = CAPACITY - 1;
		auto old = bytes{0x25}, _new = bytes{0x56};

		adapter->add(block, old);
		adapter->add(block, _new);

		bytes returned;
		adapter->get(block, returned);

		vector<pair<number, bytes>> got;
		adapter->getAll(got);
		ASSERT_EQ(1, got.size());
		ASSERT_EQ(old, returned);
	}
}

int main(int argc, char** argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
