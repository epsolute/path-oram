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
	enum TestingStorageAdapterType
	{
		StorageAdapterTypeInMemory,
		StorageAdapterTypeFileSystem,
		StorageAdapterTypeRedis,
		StorageAdapterTypeAerospike
	};

	class ORAMBigTest : public testing::TestWithParam<tuple<number, number, number, TestingStorageAdapterType, bool, bool, number>>
	{
		public:
		inline static string REDIS_HOST		= "tcp://127.0.0.1:6379";
		inline static string AEROSPIKE_HOST = "127.0.0.1";

		protected:
		unique_ptr<ORAM> oram;
		shared_ptr<AbsStorageAdapter> storage;
		shared_ptr<AbsPositionMapAdapter> map;
		shared_ptr<AbsStashAdapter> stash;

		inline static number BLOCK_SIZE;
		inline static number CAPACITY;
		inline static number ELEMENTS;
		inline static number LOG_CAPACITY;
		inline static number Z;
		inline static number BULK_LOAD;
		inline static number BATCH_SIZE;
		inline static string FILENAME = "storage.bin";
		inline static bytes KEY;

		ORAMBigTest()
		{
			KEY = getRandomBlock(KEYSIZE);

			auto [LOG_CAPACITY, Z, BLOCK_SIZE, storageType, externalPositionMap, BULK_LOAD, BATCH_SIZE] = GetParam();
			this->BLOCK_SIZE																			= BLOCK_SIZE;
			this->BULK_LOAD																				= BULK_LOAD;
			this->BATCH_SIZE																			= BATCH_SIZE;
			this->LOG_CAPACITY																			= LOG_CAPACITY;
			this->Z																						= Z;
			this->CAPACITY																				= (1 << LOG_CAPACITY);
			this->ELEMENTS																				= (CAPACITY * Z / 4) * 3;

			switch (storageType)
			{
				case StorageAdapterTypeInMemory:
					this->storage = shared_ptr<AbsStorageAdapter>(new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE, KEY, Z));
					break;
				case StorageAdapterTypeFileSystem:
					this->storage = shared_ptr<AbsStorageAdapter>(new FileSystemStorageAdapter(CAPACITY + Z, BLOCK_SIZE, KEY, FILENAME, true, Z));
					break;
				case StorageAdapterTypeRedis:
					this->storage = shared_ptr<AbsStorageAdapter>(new RedisStorageAdapter(CAPACITY + Z, BLOCK_SIZE, KEY, REDIS_HOST, true, Z));
					break;
				case StorageAdapterTypeAerospike:
					this->storage = shared_ptr<AbsStorageAdapter>(new AerospikeStorageAdapter(CAPACITY + Z, BLOCK_SIZE, KEY, AEROSPIKE_HOST, true, Z));
					break;
				default:
					throw Exception(boost::format("TestingStorageAdapterType %1% is not implemented") % storageType);
			}

			auto logCapacity = max((number)ceil(log(CAPACITY * Z) / log(2)), 3uLL);
			auto z			 = 3uLL;
			auto capacity	 = (1 << logCapacity);
			auto blockSize	 = 2 * AES_BLOCK_SIZE;
			this->map		 = !externalPositionMap ?
							shared_ptr<AbsPositionMapAdapter>(new InMemoryPositionMapAdapter(CAPACITY * Z + Z)) :
							shared_ptr<AbsPositionMapAdapter>(new ORAMPositionMapAdapter(
								make_unique<ORAM>(
									logCapacity,
									blockSize,
									z,
									make_unique<InMemoryStorageAdapter>(capacity * z + z, blockSize, bytes(), z),
									make_unique<InMemoryPositionMapAdapter>(capacity * z + z),
									make_unique<InMemoryStashAdapter>(3 * logCapacity * z))));
			this->stash = make_shared<InMemoryStashAdapter>(2 * LOG_CAPACITY * Z);

			this->oram = make_unique<ORAM>(LOG_CAPACITY, BLOCK_SIZE, Z, storage, map, stash, true, BATCH_SIZE);
		}

		~ORAMBigTest()
		{
			remove(FILENAME.c_str());
			if (get<3>(GetParam()) == StorageAdapterTypeAerospike)
			{
				static_pointer_cast<AerospikeStorageAdapter>(storage)->deleteAll();
			}
			storage.reset();
			if (get<3>(GetParam()) == StorageAdapterTypeRedis)
			{
				make_unique<sw::redis::Redis>(REDIS_HOST)->flushall();
			}
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
			if (get<3>(GetParam()) == StorageAdapterTypeFileSystem && !get<4>(GetParam()))
			{
				storeKey(KEY, "key.bin");
				storage.reset();
				KEY		= loadKey("key.bin");
				storage = make_shared<FileSystemStorageAdapter>(CAPACITY + Z, BLOCK_SIZE, KEY, FILENAME, false, Z);

				dynamic_pointer_cast<InMemoryPositionMapAdapter>(map)->storeToFile("position-map.bin");
				map.reset();
				map = make_shared<InMemoryPositionMapAdapter>(CAPACITY * Z + Z);
				dynamic_pointer_cast<InMemoryPositionMapAdapter>(map)->loadFromFile("position-map.bin");

				dynamic_pointer_cast<InMemoryStashAdapter>(stash)->storeToFile("stash.bin");
				vector<block> stashDump;
				stash->getAll(stashDump);
				auto blockSize = stashDump.size() > 0 ? stashDump[0].second.size() : 0;
				stash.reset();
				stash = make_shared<InMemoryStashAdapter>(2 * LOG_CAPACITY * Z);
				dynamic_pointer_cast<InMemoryStashAdapter>(stash)->loadFromFile("stash.bin", blockSize);

				oram.reset();
				oram = make_unique<ORAM>(LOG_CAPACITY, BLOCK_SIZE, Z, storage, map, stash, false);
			}
		}
	};

	string printTestName(testing::TestParamInfo<tuple<number, number, number, TestingStorageAdapterType, bool, bool, number>> input)
	{
		auto [LOG_CAPACITY, Z, BLOCK_SIZE, storageType, externalPositionMap, bulkLoad, batchSize] = input.param;
		auto CAPACITY																			  = (1 << LOG_CAPACITY);

		return boost::str(boost::format("i%1%i%2%i%3%i%4%i%5%i%6%i%7%i%8%") % LOG_CAPACITY % Z % BLOCK_SIZE % CAPACITY % storageType % externalPositionMap % bulkLoad % batchSize);
	}

	vector<tuple<number, number, number, TestingStorageAdapterType, bool, bool, number>> cases()
	{
		vector<tuple<number, number, number, TestingStorageAdapterType, bool, bool, number>> result = {
			{5, 3, 32, StorageAdapterTypeInMemory, false, false, 1},
			{10, 4, 64, StorageAdapterTypeInMemory, false, false, 1},
			{10, 5, 64, StorageAdapterTypeInMemory, false, false, 1},
			{10, 5, 256, StorageAdapterTypeInMemory, false, false, 1},
			{7, 4, 64, StorageAdapterTypeFileSystem, false, false, 1},
			{7, 4, 64, StorageAdapterTypeFileSystem, true, false, 1},
			{7, 4, 64, StorageAdapterTypeFileSystem, false, true, 1},
			{7, 4, 64, StorageAdapterTypeInMemory, false, true, 10},
		};

		for (auto host : vector<string>{"127.0.0.1", "redis"})
		{
			try
			{
				// test if Redis is availbale
				auto connection = "tcp://" + host + ":6379";
				make_unique<sw::redis::Redis>(connection)->ping();
				result.push_back({5, 3, 32, StorageAdapterTypeRedis, true, false, 1});
				PathORAM::ORAMBigTest::REDIS_HOST = connection;
				break;
			}
			catch (...)
			{
			}
		}

		for (auto host : vector<string>{"127.0.0.1", "aerospike"})
		{
			try
			{
				// test if Aerospike is availbale
				as_config config;
				as_config_init(&config);
				as_config_add_host(&config, host.c_str(), 3000);

				aerospike aerospike;
				aerospike_init(&aerospike, &config);

				as_error err;
				aerospike_connect(&aerospike, &err);

				if (err.code == AEROSPIKE_OK)
				{
					result.push_back({5, 3, 32, StorageAdapterTypeAerospike, true, false, 1});
					PathORAM::ORAMBigTest::AEROSPIKE_HOST = host;
					break;
				}
			}
			catch (...)
			{
			}
		}

		return result;
	};

	INSTANTIATE_TEST_SUITE_P(BigORAMSuite, ORAMBigTest, testing::ValuesIn(cases()), printTestName);

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
			oram->load(vector<block>(local.begin(), local.end()));
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
		vector<block> batch;
		for (number i = 0; i < ELEMENTS * 5; i++)
		{
			auto id	  = getRandomULong(ELEMENTS);
			auto read = getRandomULong(2) == 0;

			if (read)
			{
				// get
				batch.push_back({id, bytes()});
			}
			else
			{
				// put
				auto data = fromText(to_string(ELEMENTS + getRandomULong(ELEMENTS)), BLOCK_SIZE);
				batch.push_back({id, data});
			}

			if (i % BATCH_SIZE == 0 || i == ELEMENTS * 5 - 1)
			{
				if (batch.size() > 0)
				{
					if (batch.size() == 1)
					{
						// not using batch
						if (batch[0].second.size() == 0)
						{
							// read
							auto returned = oram->get(batch[0].first);
							EXPECT_EQ(local[batch[0].first], returned);
						}
						else
						{
							// write
							oram->put(batch[0].first, batch[0].second);
							local[batch[0].first] = batch[0].second;
						}
					}
					else
					{
						// batch enabled
						auto returned = oram->multiple(batch);
						ASSERT_EQ(batch.size(), returned.size());

						for (number j = 0; j < batch.size(); j++)
						{
							if (batch[j].second.size() == 0)
							{
								// read
								EXPECT_EQ(local[batch[j].first], returned[j]);
							}
							else
							{
								// write
								EXPECT_EQ(batch[j].second, returned[j]);
								local[batch[j].first] = batch[j].second;
							}
						}
					}

					batch.clear();
				}
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
