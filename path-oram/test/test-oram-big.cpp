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
	class ORAMBigTest : public testing::TestWithParam<tuple<number, number, number, bool, bool, bool>>
	{
		protected:
		ORAM* oram;
		AbsStorageAdapter* storage;
		AbsPositionMapAdapter* map;
		AbsStashAdapter* stash;

		inline static number BLOCK_SIZE;
		inline static number CAPACITY;
		inline static number ELEMENTS;
		inline static number LOG_CAPACITY;
		inline static number Z;
		inline static number BULK_LOAD;
		inline static string FILENAME = "storage.bin";
		inline static bytes KEY;

		ORAMBigTest()
		{
			KEY = getRandomBlock(KEYSIZE);

			auto [LOG_CAPACITY, Z, BLOCK_SIZE, externalStorage, externalPositionMap, BULK_LOAD] = GetParam();
			this->BLOCK_SIZE																	= BLOCK_SIZE;
			this->BULK_LOAD																		= BULK_LOAD;
			this->LOG_CAPACITY																	= LOG_CAPACITY;
			this->Z																				= Z;
			this->CAPACITY																		= (1 << LOG_CAPACITY) * Z;
			this->ELEMENTS																		= (CAPACITY / 4) * 3;

			this->storage = !externalStorage ?
								(AbsStorageAdapter*)new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE, KEY) :
								(AbsStorageAdapter*)new FileSystemStorageAdapter(CAPACITY + Z, BLOCK_SIZE, KEY, FILENAME, true);

			auto logCapacity = max((number)ceil(log(CAPACITY) / log(2)), 3uLL);
			auto z			 = 3uLL;
			auto capacity	 = (1 << logCapacity) * z;
			auto blockSize	 = 2 * AES_BLOCK_SIZE;
			this->map		 = !externalPositionMap ?
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

			this->oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, storage, map, stash);
		}

		~ORAMBigTest() override
		{
			auto externalPositionMap = get<4>(GetParam());

			if (externalPositionMap)
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

		/**
		 * @brief emulate controlled crash
		 *
		 * Write all components to binary files and recreate them fro the files.
		 * Will do so only if storage is FS and others are InMemory.
		 */
		void disaster()
		{
			// if using FS storage and in-memory position map and stash
			// simulate disaster
			if (get<3>(GetParam()) && !get<4>(GetParam()))
			{
				storeKey(KEY, "key.bin");
				delete storage;
				KEY		= loadKey("key.bin");
				storage = new FileSystemStorageAdapter(CAPACITY + Z, BLOCK_SIZE, KEY, FILENAME, false);

				((InMemoryPositionMapAdapter*)map)->storeToFile("position-map.bin");
				delete map;
				map = new InMemoryPositionMapAdapter(CAPACITY + Z);
				((InMemoryPositionMapAdapter*)map)->loadFromFile("position-map.bin");

				((InMemoryStashAdapter*)stash)->storeToFile("stash.bin");
				auto blockSize = stash->getAll().size() > 0 ? stash->getAll()[0].second.size() : 0;
				delete stash;
				stash = new InMemoryStashAdapter(2 * LOG_CAPACITY * Z);
				((InMemoryStashAdapter*)stash)->loadFromFile("stash.bin", blockSize);

				delete oram;
				oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, storage, map, stash, false);
			}
		}
	};

	string printTestName(testing::TestParamInfo<tuple<number, number, number, bool, bool, bool>> input)
	{
		auto [LOG_CAPACITY, Z, BLOCK_SIZE, externalStorage, externalPositionMap, bulkLoad] = input.param;
		auto CAPACITY																	   = (1 << LOG_CAPACITY) * Z;

		return boost::str(boost::format("i%1%i%2%i%3%i%4%i%5%i%6%i%7%") % LOG_CAPACITY % Z % BLOCK_SIZE % CAPACITY % externalStorage % externalPositionMap % bulkLoad);
	}

	tuple<number, number, number, bool, bool, bool> cases[] = {
		{5, 3, 32, false, false, false},
		{10, 4, 64, false, false, false},
		{10, 5, 64, false, false, false},
		{10, 5, 256, false, false, false},
		{7, 4, 64, true, false, false},
		{7, 4, 64, true, true, false},
		{7, 4, 64, false, false, true},
	};

	INSTANTIATE_TEST_SUITE_P(BigORAMSuite, ORAMBigTest, testing::ValuesIn(cases), printTestName);

	TEST_P(ORAMBigTest, Simulation)
	{
		unordered_map<number, bytes> local;
		local.reserve(ELEMENTS);

		// generate data
		for (number id = 0; id < ELEMENTS; id++)
		{
			auto data = fromText(to_string(id), BLOCK_SIZE);
			local[id] = data;
		}

		// put / load all
		if (get<5>(GetParam()))
		{
			oram->load(vector<pair<number, bytes>>(local.begin(), local.end()));
		}
		else
		{
			for (auto record : local)
			{
				oram->put(record.first, record.second);
			}
		}

		disaster();

		// get all
		for (number id = 0; id < ELEMENTS; id++)
		{
			auto returned = oram->get(id);
			EXPECT_EQ(local[id], returned);
		}

		disaster();

		// random operations
		for (number i = 0; i < ELEMENTS * 5; i++)
		{
			auto id	  = getRandomULong(ELEMENTS);
			auto read = getRandomULong(2) == 0;
			if (read)
			{
				// get
				auto returned = oram->get(id);
				EXPECT_EQ(local[id], returned);
			}
			else
			{
				auto data = fromText(to_string(ELEMENTS + getRandomULong(ELEMENTS)), BLOCK_SIZE);
				local[id] = data;
				oram->put(id, data);
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
