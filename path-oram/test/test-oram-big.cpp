#include "definitions.h"
#include "oram.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <boost/format.hpp>
#include <math.h>
#include <openssl/aes.h>

using namespace std;

namespace PathORAM
{
	class ORAMBigTest : public testing::TestWithParam<tuple<ulong, ulong, ulong, bool>>
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
			auto [LOG_CAPACITY, Z, BLOCK_SIZE, useExternal] = GetParam();
			this->BLOCK_SIZE								= BLOCK_SIZE;
			this->CAPACITY									= (1 << LOG_CAPACITY) * Z;
			this->ELEMENTS									= (CAPACITY / 4) * 3;

			this->storage = !useExternal ?
								(AbsStorageAdapter*)new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE, bytes()) :
								(AbsStorageAdapter*)new FileSystemStorageAdapter(CAPACITY + Z, BLOCK_SIZE, bytes(), "storage.bin", true);

			auto logCapacity = max((ulong)ceil(log(CAPACITY) / log(2)), 3uLL);
			auto z			 = 3uLL;
			auto capacity	= (1 << logCapacity) * z;
			auto blockSize   = 2 * AES_BLOCK_SIZE;
			this->map		 = !useExternal ?
							(AbsPositionMapAdapter*)new InMemoryPositionMapAdapter(CAPACITY + Z) :
							(AbsPositionMapAdapter*)new ORAMPositionMapAdapter(
								new ORAM(
									logCapacity,
									blockSize,
									z,
									new InMemoryStorageAdapter(capacity + z, blockSize, bytes()),
									new InMemoryPositionMapAdapter(capacity + z),
									new InMemoryStashAdapter(3 * logCapacity * z)));
			this->stash = new InMemoryStashAdapter(2 * LOG_CAPACITY * Z);

			this->oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, this->storage, this->map, this->stash);
		}

		~ORAMBigTest() override
		{
			auto useExternal = get<3>(GetParam());

			if (useExternal)
			{
				delete ((ORAMPositionMapAdapter*)map)->oram->map;
				delete ((ORAMPositionMapAdapter*)map)->oram->storage;
				delete ((ORAMPositionMapAdapter*)map)->oram->stash;
			}

			delete oram;
			delete storage;
			delete map;
			delete stash;
		}
	};

	string printTestName(testing::TestParamInfo<tuple<ulong, ulong, ulong, bool>> input)
	{
		auto [LOG_CAPACITY, Z, BLOCK_SIZE, useExternal] = input.param;
		auto CAPACITY									= (1 << LOG_CAPACITY) * Z;

		return boost::str(boost::format("i%1%i%2%i%3%i%4%i%5%") % LOG_CAPACITY % Z % BLOCK_SIZE % CAPACITY % useExternal);
	}

	tuple<ulong, ulong, ulong, bool> cases[] = {
		{5, 3, 32, false},
		{10, 4, 64, false},
		{10, 5, 64, false},
		{10, 5, 256, false},
		{7, 4, 64, true},
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
