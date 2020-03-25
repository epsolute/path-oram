#include "definitions.h"
#include "oram.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <boost/format.hpp>

using namespace std;

namespace PathORAM
{
	class ORAMTest : public ::testing::Test
	{
		public:
		inline static const ulong LOG_CAPACITY = 5;
		inline static const ulong Z			   = 3;
		inline static const ulong BLOCK_SIZE   = 32;

		inline static const ulong CAPACITY = (1 << LOG_CAPACITY) * Z;

		protected:
		ORAM* oram;
		AbsStorageAdapter* storage = new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE);
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
			for (ulong i = 0; i < CAPACITY + Z; i++)
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

	TEST_F(ORAMTest, BucketFromLevelLeaf)
	{
		vector<pair<ulong, vector<ulong>>> tests =
			{
				{6, {1, 2, 5, 11, 22}},
				{8, {1, 3, 6, 12, 24}},
				{14, {1, 3, 7, 15, 30}},
			};

		for (auto test : tests)
		{
			for (ulong level = 0; level < LOG_CAPACITY; level++)
			{
				EXPECT_EQ(test.second[level], this->oram->bucketForLevelLeaf(level, test.first));
			}
		}
	}

	TEST_F(ORAMTest, CanInclude)
	{
		vector<tuple<ulong, ulong, ulong, bool>> tests =
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

	TEST_F(ORAMTest, ConsistencyCheck)
	{
		ASSERT_NO_THROW(this->oram->checkConsistency());

		auto const block = CAPACITY - 1;
		this->map->set(block, 1);
		this->storage->set(5, {block, bytes()});

		ASSERT_ANY_THROW(this->oram->checkConsistency());
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
			for (ulong i = 0; i < this->Z; i++)
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
		this->oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE - sizeof(ulong)));
	}

	TEST_F(ORAMTest, GetPutSame)
	{
		this->oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE - sizeof(ulong)));
		auto returned = this->oram->get(CAPACITY - 1);

		ASSERT_EQ("hello", toText(returned, BLOCK_SIZE));
	}

	TEST_F(ORAMTest, PutMany)
	{
		for (ulong id = 0; id < CAPACITY; id++)
		{
			this->oram->put(id, fromText(to_string(id), BLOCK_SIZE - sizeof(ulong)));
		}

		for (ulong id = 0; id < CAPACITY; id++)
		{
			auto found = false;
			for (ulong location = 0; location < CAPACITY; location++)
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
		for (ulong id = 0; id < CAPACITY; id++)
		{
			this->oram->put(id, fromText(to_string(id), BLOCK_SIZE - sizeof(ulong)));
		}

		for (ulong id = 0; id < CAPACITY; id++)
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

		for (ulong id = 0; id < CAPACITY; id++)
		{
			this->oram->put(id, fromText(to_string(id), BLOCK_SIZE - sizeof(ulong)));
			puts.push_back(this->stash->getAll().size());
		}

		for (ulong id = 0; id < CAPACITY; id++)
		{
			this->oram->get(id);
			gets.push_back(this->stash->getAll().size());
		}

		EXPECT_GE(LOG_CAPACITY * Z * 2, *max_element(gets.begin(), gets.end()));
		EXPECT_EQ(0, *min_element(puts.begin(), puts.end()));
	}

	class ORAMBigTest : public testing::TestWithParam<tuple<ulong, ulong, ulong>>
	{
		protected:
		ORAM* oram;
		AbsStorageAdapter* storage;
		AbsPositionMapAdapter* map;
		AbsStashAdapter* stash;

		inline static ulong BLOCK_SIZE;
		inline static ulong CAPACITY;
		inline static ulong ELEMENTS;

		ORAMBigTest()
		{
			auto [LOG_CAPACITY, Z, BLOCK_SIZE] = GetParam();
			this->BLOCK_SIZE				   = BLOCK_SIZE;
			CAPACITY						   = (1 << LOG_CAPACITY) * Z;
			ELEMENTS						   = (CAPACITY / 4) * 3;

			this->storage = new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE);
			this->map	 = new InMemoryPositionMapAdapter(CAPACITY + Z);
			this->stash   = new InMemoryStashAdapter(2 * LOG_CAPACITY * Z);
			this->oram	= new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, this->storage, this->map, this->stash);
		}

		~ORAMBigTest() override
		{
			delete oram;
			delete storage;
			delete map;
			delete stash;
		}
	};

	string printTestName(testing::TestParamInfo<tuple<ulong, ulong, ulong>> input)
	{
		auto [LOG_CAPACITY, Z, BLOCK_SIZE] = input.param;
		auto CAPACITY					   = (1 << LOG_CAPACITY) * Z;

		return boost::str(boost::format("i%1%i%2%i%3%i%4%") % LOG_CAPACITY % Z % BLOCK_SIZE % CAPACITY);
	}

	tuple<ulong, ulong, ulong> cases[] = {
		{5, 3, 32},
		{10, 4, 64},
		{10, 5, 64},
		{10, 5, 256},
	};

	INSTANTIATE_TEST_SUITE_P(BigORAMSuite, ORAMBigTest, testing::ValuesIn(cases), printTestName);

	TEST_P(ORAMBigTest, Simulation)
	{
		unordered_map<ulong, bytes> local;
		local.reserve(ELEMENTS);

		// put all
		for (ulong id = 0; id < ELEMENTS; id++)
		{
			auto data = fromText(to_string(id), BLOCK_SIZE - sizeof(ulong));
			local[id] = data;
			this->oram->put(id, data);
		}

		// get all
		for (ulong id = 0; id < ELEMENTS; id++)
		{
			auto returned = this->oram->get(id);
			EXPECT_EQ(local[id], returned);
		}

		// random operations
		for (ulong i = 0; i < ELEMENTS * 5; i++)
		{
			auto id   = getRandomULong(ELEMENTS);
			auto read = getRandomULong(2) == 0;
			if (read)
			{
				// get
				auto returned = this->oram->get(id);
				EXPECT_EQ(local[id], returned);
			}
			else
			{
				auto data = fromText(to_string(ELEMENTS + getRandomULong(ELEMENTS)), BLOCK_SIZE - sizeof(ulong));
				local[id] = data;
				this->oram->put(id, data);
			}
		}
	}
}

int main(int argc, char** argv)
{
	PathORAM::seedRandom(0x13);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
