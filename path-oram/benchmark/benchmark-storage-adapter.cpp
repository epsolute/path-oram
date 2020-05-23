#include "definitions.h"
#include "storage-adapter.hpp"

#include <benchmark/benchmark.h>
#include <boost/format.hpp>
#include <fstream>

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

	class StorageAdapterBenchmark : public ::benchmark::Fixture
	{
		public:
		inline static const number BLOCK_SIZE = 32;
		inline static const string FILE_NAME  = "storage.bin";
		inline static number CAPACITY		  = 1 << 17;
		inline static string REDIS_HOST		  = "tcp://127.0.0.1:6379";
		inline static string AEROSPIKE_HOST	  = "127.0.0.1";

		protected:
		unique_ptr<AbsStorageAdapter> adapter;

		~StorageAdapterBenchmark() override
		{
			remove(FILE_NAME.c_str());
		}

		void Configure(TestingStorageAdapterType type)
		{
			switch (type)
			{
				case StorageAdapterTypeInMemory:
					adapter = make_unique<InMemoryStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), 1);
					break;
				case StorageAdapterTypeFileSystem:
					adapter = make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), FILE_NAME, true, 1);
					break;
				case StorageAdapterTypeRedis:
					adapter = make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), REDIS_HOST, false, 1);
					break;
				case StorageAdapterTypeAerospike:
					adapter = make_unique<AerospikeStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), AEROSPIKE_HOST, false, 1);
					break;
				default:
					throw Exception(boost::format("TestingStorageAdapterType %1% is not implemented") % type);
			}
		}
	};

	BENCHMARK_DEFINE_F(StorageAdapterBenchmark, Write)
	(benchmark::State& state)
	{
		Configure((TestingStorageAdapterType)state.range(0));
		auto batch = state.range(1);

		bucket toWrite = {{5uLL, bytes()}};

		number location = 0;
		for (auto _ : state)
		{
			if (batch == 1)
			{
				adapter->set((location * (1 << 10)) % CAPACITY, toWrite);
			}
			else
			{
				vector<pair<number, bucket>> writes;
				writes.resize(batch);
				for (auto i = 0; i < batch; i++)
				{
					writes[i] = {((location + i) * (1 << 10)) % CAPACITY, toWrite};
				}
				adapter->set(writes);
			}

			location++;
		}
	}

	BENCHMARK_DEFINE_F(StorageAdapterBenchmark, Read)
	(benchmark::State& state)
	{
		Configure((TestingStorageAdapterType)state.range(0));
		auto batch = state.range(1);

		bucket toWrite = {{5uLL, bytes()}};
		vector<block> read;

		for (number i = 0; i < CAPACITY; i++)
		{
			adapter->set(i, toWrite);
		}

		number location = 0;
		for (auto _ : state)
		{
			if (batch == 1)
			{
				adapter->get((location * (1 << 10)) % CAPACITY, read);
			}
			else
			{
				vector<number> reads;
				reads.resize(batch);
				for (auto i = 0; i < batch; i++)
				{
					reads[i] = ((location + i) * (1 << 10)) % CAPACITY;
				}
				adapter->get(reads, read);
			}
			read.clear();
		}
	}

	static void arguments(benchmark::internal::Benchmark* b)
	{
		b
			->Args({StorageAdapterTypeInMemory, 1})
			->Args({StorageAdapterTypeInMemory, 16})
			->Args({StorageAdapterTypeFileSystem, 1})
			->Args({StorageAdapterTypeFileSystem, 16});

		auto iterations = 1 << 15;

		for (auto host : vector<string>{"127.0.0.1", "redis"})
		{
			try
			{
				// test if Redis is availbale
				auto connection = "tcp://" + host + ":6379";
				make_unique<sw::redis::Redis>(connection)->ping();
				PathORAM::StorageAdapterBenchmark::REDIS_HOST = connection;

				b
					->Args({StorageAdapterTypeRedis, 1})
					->Args({StorageAdapterTypeRedis, 16});
				iterations									= 1 << 10;
				PathORAM::StorageAdapterBenchmark::CAPACITY = 1 << 12;

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
					PathORAM::StorageAdapterBenchmark::AEROSPIKE_HOST = host;
					b
						->Args({StorageAdapterTypeAerospike, 1})
						->Args({StorageAdapterTypeAerospike, 16});
					iterations									= 1 << 10;
					PathORAM::StorageAdapterBenchmark::CAPACITY = 1 << 12;

					break;
				}
			}
			catch (...)
			{
			}
		}

		b->Iterations(iterations);
	}

	BENCHMARK_REGISTER_F(StorageAdapterBenchmark, Write)
		->Apply(arguments)
		->Unit(benchmark::kMicrosecond);

	BENCHMARK_REGISTER_F(StorageAdapterBenchmark, Read)
		->Apply(arguments)
		->Unit(benchmark::kMicrosecond);
}

BENCHMARK_MAIN();
