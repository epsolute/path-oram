#include "definitions.h"
#include "storage-adapter.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <boost/format.hpp>
#include <fstream>
#include <openssl/aes.h>

using namespace std;

namespace PathORAM
{
	enum TestingStorageAdapterType
	{
		StorageAdapterTypeInMemory,
		StorageAdapterTypeFileSystem,
		StorageAdapterTypeRedis,
		StorageAdapterTypeAerospike,
	};

	class StorageAdapterTest : public testing::TestWithParam<TestingStorageAdapterType>
	{
		public:
		inline static const number CAPACITY	  = 10;
		inline static const number BLOCK_SIZE = 32;
		inline static const string FILE_NAME  = "storage.bin";
		inline static string REDIS_HOST		  = "tcp://127.0.0.1:6379";
		inline static string AEROSPIKE_HOST	  = "127.0.0.1";

		protected:
		unique_ptr<AbsStorageAdapter> adapter;

		StorageAdapterTest()
		{
			auto type = GetParam();
			switch (type)
			{
				case StorageAdapterTypeInMemory:
					adapter = make_unique<InMemoryStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes());
					break;
				case StorageAdapterTypeFileSystem:
					adapter = make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), FILE_NAME, true);
					break;
				case StorageAdapterTypeRedis:
					adapter = make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), REDIS_HOST, true);
					break;
				case StorageAdapterTypeAerospike:
					adapter = make_unique<AerospikeStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), AEROSPIKE_HOST, true);
					break;
				default:
					throw Exception(boost::format("TestingStorageAdapterType %1% is not implemented") % type);
			}
		}

		~StorageAdapterTest() override
		{
			remove(FILE_NAME.c_str());
			adapter.reset();
			if (GetParam() == StorageAdapterTypeRedis)
			{
				make_unique<sw::redis::Redis>(REDIS_HOST)->flushall();
			}
		}
	};

	TEST_P(StorageAdapterTest, Initialization)
	{
		SUCCEED();
	}

	TEST_P(StorageAdapterTest, RecoverAfterCrash)
	{
		auto param		   = GetParam();
		auto createAdapter = [param](string filename, bool override, bytes key) -> unique_ptr<AbsStorageAdapter> {
			switch (param)
			{
				case StorageAdapterTypeFileSystem:
					return make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, key, filename, override);
				case StorageAdapterTypeRedis:
					return make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, key, REDIS_HOST, override);
				case StorageAdapterTypeAerospike:
					return make_unique<AerospikeStorageAdapter>(CAPACITY, BLOCK_SIZE, key, AEROSPIKE_HOST, override);
				default:
					throw Exception(boost::format("TestingStorageAdapterType %1% is not persistent") % param);
			}
		};

		if (GetParam() != StorageAdapterTypeInMemory)
		{
			auto data = fromText("hello", BLOCK_SIZE);
			auto key  = getRandomBlock(KEYSIZE);

			string filename = "tmp.bin";
			auto storage	= createAdapter(filename, true, key);

			storage->set(CAPACITY - 1, {5, data});
			ASSERT_EQ(data, storage->get(CAPACITY - 1).second);
			storage.reset();

			storage = createAdapter(filename, false, key);

			ASSERT_EQ(data, storage->get(CAPACITY - 1).second);

			remove(filename.c_str());
		}
		else
		{
			SUCCEED();
		}
	}

	TEST_P(StorageAdapterTest, CrashAtInitialization)
	{
		switch (GetParam())
		{
			case StorageAdapterTypeFileSystem:
				ASSERT_ANY_THROW(make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), "tmp.bin", false));
				break;
			case StorageAdapterTypeRedis:
				ASSERT_ANY_THROW(make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), "error", false));
				break;
			case StorageAdapterTypeAerospike:
				ASSERT_ANY_THROW(make_unique<AerospikeStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), "error", false));
				break;
			default:
				SUCCEED();
				break;
		}
	}

	TEST_P(StorageAdapterTest, InputsCheck)
	{
		ASSERT_ANY_THROW(make_unique<InMemoryStorageAdapter>(CAPACITY, AES_BLOCK_SIZE, bytes()));
		ASSERT_ANY_THROW(make_unique<InMemoryStorageAdapter>(CAPACITY, AES_BLOCK_SIZE * 3 - 1, bytes()));
	}

	TEST_P(StorageAdapterTest, ReadWriteNoCrash)
	{
		EXPECT_NO_THROW({
			adapter->set(CAPACITY - 1, {5uLL, bytes()});
			adapter->get(CAPACITY - 2);
		});
	}

	TEST_P(StorageAdapterTest, ReadEmpty)
	{
		auto data = adapter->get(CAPACITY - 2).second;
		ASSERT_EQ(BLOCK_SIZE, data.size());
	}

	TEST_P(StorageAdapterTest, IdOutOfBounds)
	{
		ASSERT_ANY_THROW(adapter->get(CAPACITY + 1));
		ASSERT_ANY_THROW(adapter->set(CAPACITY + 1, {5uLL, bytes()}));
	}

	TEST_P(StorageAdapterTest, DataTooBig)
	{
		ASSERT_ANY_THROW(adapter->set(CAPACITY - 1, {5uLL, bytes(BLOCK_SIZE + 1, 0x08)}));
	}

	TEST_P(StorageAdapterTest, ReadWhatWasWritten)
	{
		auto data = bytes{0xa8};
		auto id	  = 5uLL;

		adapter->set(CAPACITY - 1, {id, data});
		auto [returnedId, returnedData] = adapter->get(CAPACITY - 1);

		data.resize(BLOCK_SIZE, 0x00);

		ASSERT_EQ(id, returnedId);
		ASSERT_EQ(data, returnedData);
	}

	// if get/set internal for batching are implemented, they are used
	// byt get/set internal single still has to work
	TEST_P(StorageAdapterTest, GetSetInternal)
	{
		auto data = bytes{0xa8};
		data.resize(BLOCK_SIZE + 2 * AES_BLOCK_SIZE);

		adapter->setInternal(CAPACITY - 1, data);
		auto returned = adapter->getInternal(CAPACITY - 1);

		ASSERT_EQ(data, returned);
	}

	TEST_P(StorageAdapterTest, OverrideData)
	{
		auto id	  = 5;
		auto data = bytes{0xa8};
		data.resize(BLOCK_SIZE, 0x00);

		adapter->set(CAPACITY - 1, {id, data});
		data[0] = 0x56;
		id		= 6;

		adapter->set(CAPACITY - 1, {id, data});

		auto [returnedId, returnedData] = adapter->get(CAPACITY - 1);

		ASSERT_EQ(id, returnedId);
		ASSERT_EQ(data, returnedData);
	}

	TEST_P(StorageAdapterTest, InitializeToEmpty)
	{
		for (number i = 0; i < CAPACITY; i++)
		{
			auto expected = bytes();
			expected.resize(BLOCK_SIZE);
			ASSERT_EQ(ULONG_MAX, adapter->get(i).first);
			ASSERT_EQ(expected, adapter->get(i).second);
		}
	}

	TEST_P(StorageAdapterTest, BatchReadWrite)
	{
		const auto runs = 3;

		vector<pair<number, pair<number, bytes>>> writes;
		vector<number> reads;
		for (auto i = 0; i < runs; i++)
		{
			writes.push_back({CAPACITY - 4 + i, {i, bytes{(uchar)(0x05 + i)}}});
			reads.push_back(CAPACITY - 4 + i);
		}
		adapter->set(writes);

		auto read = adapter->get(reads);

		for (auto i = 0; i < runs; i++)
		{
			ASSERT_EQ(i, read[i].first);
			auto expected = bytes{(uchar)(0x05 + i)};
			expected.resize(BLOCK_SIZE);
			ASSERT_EQ(expected, read[i].second);
		}
	}

	string printTestName(testing::TestParamInfo<TestingStorageAdapterType> input)
	{
		switch (input.param)
		{
			case StorageAdapterTypeInMemory:
				return "InMemory";
			case StorageAdapterTypeFileSystem:
				return "FileSystem";
			case StorageAdapterTypeRedis:
				return "Redis";
			case StorageAdapterTypeAerospike:
				return "Aerospike";
			default:
				throw Exception(boost::format("TestingStorageAdapterType %1% is not implemented") % input.param);
		}
	}

	vector<TestingStorageAdapterType> cases()
	{
		vector<TestingStorageAdapterType> result = {StorageAdapterTypeFileSystem, StorageAdapterTypeInMemory};
		for (auto host : vector<string>{"127.0.0.1", "redis"})
		{
			try
			{
				// test if Redis is availbale
				auto connection = "tcp://" + host + ":6379";
				make_unique<sw::redis::Redis>(connection)->ping();
				result.push_back(StorageAdapterTypeRedis);
				PathORAM::StorageAdapterTest::REDIS_HOST = connection;
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
					result.push_back(StorageAdapterTypeAerospike);
					PathORAM::StorageAdapterTest::AEROSPIKE_HOST = host;
					break;
				}
			}
			catch (...)
			{
			}
		}

		return result;
	};

	INSTANTIATE_TEST_SUITE_P(StorageAdapterSuite, StorageAdapterTest, testing::ValuesIn(cases()), printTestName);
}

int main(int argc, char** argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
