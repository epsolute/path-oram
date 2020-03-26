#include "definitions.h"
#include "oram.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <boost/format.hpp>

using namespace std;

namespace PathORAM
{
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

			this->storage = new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE, bytes());
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
			auto data = fromText(to_string(id), BLOCK_SIZE);
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
				auto data = fromText(to_string(ELEMENTS + getRandomULong(ELEMENTS)), BLOCK_SIZE);
				local[id] = data;
				this->oram->put(id, data);
			}
		}
	}
}

int main(int argc, char** argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
