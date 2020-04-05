#include "definitions.h"
#include "oram.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"

using namespace std;

namespace PathORAM
{
	class ORAMTest : public ::testing::Test
	{
		public:
		inline static const number LOG_CAPACITY = 5;
		inline static const number Z			= 3;
		inline static const number BLOCK_SIZE	= 32;

		inline static const number CAPACITY = (1 << LOG_CAPACITY) * Z;

		protected:
		ORAM* oram;
		AbsStorageAdapter* storage = new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE, bytes());
		AbsPositionMapAdapter* map = new InMemoryPositionMapAdapter(CAPACITY + Z);
		AbsStashAdapter* stash	   = new InMemoryStashAdapter(3 * LOG_CAPACITY * Z);

		ORAMTest()
		{
			this->oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, storage, map, stash);
		}

		~ORAMTest() override
		{
			delete oram;
			delete storage;
			delete map;
			delete stash;
		}

		void populateStorage()
		{
			for (number i = 0; i < CAPACITY + Z; i++)
			{
				storage->set(i, {i, bytes()});
			}
		}
	};

	TEST_F(ORAMTest, Helpers)
	{
		ASSERT_EQ(toText(fromText("hello", BLOCK_SIZE), BLOCK_SIZE), "hello");
		ASSERT_EQ(BLOCK_SIZE, fromText("hello", BLOCK_SIZE).size());
	}

	TEST_F(ORAMTest, Initialization)
	{
		SUCCEED();
	}

	TEST_F(ORAMTest, InitializationShorthand)
	{
		ASSERT_NO_THROW({
			auto oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z);
			delete oram;
		});
	}

	TEST_F(ORAMTest, BucketFromLevelLeaf)
	{
		vector<pair<number, vector<number>>> tests =
			{
				{6, {1, 2, 5, 11, 22}},
				{8, {1, 3, 6, 12, 24}},
				{14, {1, 3, 7, 15, 30}},
			};

		for (auto test : tests)
		{
			for (number level = 0; level < LOG_CAPACITY; level++)
			{
				EXPECT_EQ(test.second[level], oram->bucketForLevelLeaf(level, test.first));
			}
		}
	}

	TEST_F(ORAMTest, CanInclude)
	{
		vector<tuple<number, number, number, bool>> tests =
			{
				{8, 11, 2, true},
				{8, 11, 3, false},
				{0, 11, 0, true},
				{0, 11, 1, false},
				{0, 11, 2, false},
			};

		for (auto test : tests)
		{
			EXPECT_EQ(get<3>(test), oram->canInclude(get<0>(test), get<1>(test), get<2>(test)));
		}
	}

	TEST_F(ORAMTest, ReadPath)
	{
		populateStorage();

		EXPECT_EQ(0, stash->getAll().size());

		oram->readPath(10uLL);

		EXPECT_EQ(LOG_CAPACITY * Z, stash->getAll().size());

		vector<int> expected = {1, 3, 6, 13, 26};

		for (auto block : expected)
		{
			for (number i = 0; i < Z; i++)
			{
				auto id = block * Z + i;
				EXPECT_TRUE(stash->exists(id));
			}
		}
	}

	TEST_F(ORAMTest, GetNoException)
	{
		oram->get(CAPACITY - 1);
	}

	TEST_F(ORAMTest, PutNoException)
	{
		oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE));
	}

	TEST_F(ORAMTest, GetPutSame)
	{
		oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE));
		auto returned = oram->get(CAPACITY - 1);

		ASSERT_EQ("hello", toText(returned, BLOCK_SIZE));
	}

	TEST_F(ORAMTest, PutMany)
	{
		for (number id = 0; id < CAPACITY; id++)
		{
			oram->put(id, fromText(to_string(id), BLOCK_SIZE));
		}

		for (number id = 0; id < CAPACITY; id++)
		{
			auto found = false;
			for (number location = 0; location < CAPACITY; location++)
			{
				if (storage->get(location).first == id)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				found = stash->exists(id);
			}
			EXPECT_TRUE(found);
		}
	}

	TEST_F(ORAMTest, PutGetMany)
	{
		for (number id = 0; id < CAPACITY; id++)
		{
			oram->put(id, fromText(to_string(id), BLOCK_SIZE));
		}

		for (number id = 0; id < CAPACITY; id++)
		{
			auto returned = oram->get(id);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, BulkLoad)
	{
		vector<pair<number, bytes>> batch;
		for (number id = 0; id < 3 * CAPACITY / 4; id++)
		{
			batch.push_back({id, fromText(to_string(id), BLOCK_SIZE)});
		}

		oram->load(batch);

		for (number id = 0; id < 3 * CAPACITY / 4; id++)
		{
			auto returned = oram->get(id);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, BulkLoadTooMany)
	{
		vector<pair<number, bytes>> batch;
		for (number id = 0; id < CAPACITY + 5; id++)
		{
			batch.push_back({id, fromText(to_string(id), BLOCK_SIZE)});
		}

		ASSERT_ANY_THROW(oram->load(batch));
	}

	TEST_F(ORAMTest, StashUsage)
	{
		vector<int> puts, gets;
		puts.reserve(CAPACITY);
		gets.reserve(CAPACITY);

		for (number id = 0; id < CAPACITY; id++)
		{
			oram->put(id, fromText(to_string(id), BLOCK_SIZE));
			puts.push_back(stash->getAll().size());
		}

		for (number id = 0; id < CAPACITY; id++)
		{
			oram->get(id);
			gets.push_back(stash->getAll().size());
		}

		EXPECT_GE(LOG_CAPACITY * Z * 2, *max_element(gets.begin(), gets.end()));
		EXPECT_EQ(0, *min_element(puts.begin(), puts.end()));
	}
}

int main(int argc, char** argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
