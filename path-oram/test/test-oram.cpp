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
		inline static const number Z			   = 3;
		inline static const number BLOCK_SIZE   = 32;

		inline static const number CAPACITY = (1 << LOG_CAPACITY) * Z;

		protected:
		ORAM* oram;
		AbsStorageAdapter* storage = new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE, bytes());
		AbsPositionMapAdapter* map = new InMemoryPositionMapAdapter(CAPACITY + Z);
		AbsStashAdapter* stash	 = new InMemoryStashAdapter(3 * LOG_CAPACITY * Z);

		ORAMTest()
		{
			this->oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, this->storage, this->map, this->stash);
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
				this->storage->set(i, {i, bytes()});
			}
		}
	};

	TEST_F(ORAMTest, Helpers)
	{
		ASSERT_EQ(toText(fromText("hello", BLOCK_SIZE), BLOCK_SIZE), "hello");
		ASSERT_EQ(this->BLOCK_SIZE, fromText("hello", BLOCK_SIZE).size());
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
				EXPECT_EQ(test.second[level], this->oram->bucketForLevelLeaf(level, test.first));
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
			EXPECT_EQ(get<3>(test), this->oram->canInclude(get<0>(test), get<1>(test), get<2>(test)));
		}
	}

	TEST_F(ORAMTest, ReadPath)
	{
		this->populateStorage();

		EXPECT_EQ(0, this->stash->getAll().size());

		this->oram->readPath(10uLL);

		EXPECT_EQ(this->LOG_CAPACITY * this->Z, this->stash->getAll().size());

		vector<int> expected = {1, 3, 6, 13, 26};

		for (auto block : expected)
		{
			for (number i = 0; i < this->Z; i++)
			{
				auto id = block * this->Z + i;
				EXPECT_TRUE(this->stash->exists(id));
			}
		}
	}

	TEST_F(ORAMTest, GetNoException)
	{
		this->oram->get(CAPACITY - 1);
	}

	TEST_F(ORAMTest, PutNoException)
	{
		this->oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE));
	}

	TEST_F(ORAMTest, GetPutSame)
	{
		this->oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE));
		auto returned = this->oram->get(CAPACITY - 1);

		ASSERT_EQ("hello", toText(returned, BLOCK_SIZE));
	}

	TEST_F(ORAMTest, PutMany)
	{
		for (number id = 0; id < CAPACITY; id++)
		{
			this->oram->put(id, fromText(to_string(id), BLOCK_SIZE));
		}

		for (number id = 0; id < CAPACITY; id++)
		{
			auto found = false;
			for (number location = 0; location < CAPACITY; location++)
			{
				if (this->storage->get(location).first == id)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				found = this->stash->exists(id);
			}
			EXPECT_TRUE(found);
		}
	}

	TEST_F(ORAMTest, PutGetMany)
	{
		for (number id = 0; id < CAPACITY; id++)
		{
			this->oram->put(id, fromText(to_string(id), BLOCK_SIZE));
		}

		for (number id = 0; id < CAPACITY; id++)
		{
			auto returned = this->oram->get(id);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, StashUsage)
	{
		vector<int> puts, gets;
		puts.reserve(CAPACITY);
		gets.reserve(CAPACITY);

		for (number id = 0; id < CAPACITY; id++)
		{
			this->oram->put(id, fromText(to_string(id), BLOCK_SIZE));
			puts.push_back(this->stash->getAll().size());
		}

		for (number id = 0; id < CAPACITY; id++)
		{
			this->oram->get(id);
			gets.push_back(this->stash->getAll().size());
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
