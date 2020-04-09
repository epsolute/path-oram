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
		StorageAdapterTypeRedis
	};

	class StorageAdapterBenchmark : public ::benchmark::Fixture
	{
		public:
		inline static const number CAPACITY	  = 1 << 17;
		inline static const number BLOCK_SIZE = 32;
		inline static const string FILE_NAME  = "storage.bin";
		inline static const string REDIS_HOST = "tcp://127.0.0.1:6379";

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
					adapter = make_unique<InMemoryStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes());
					break;
				case StorageAdapterTypeFileSystem:
					adapter = make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), FILE_NAME, true);
					break;
				case StorageAdapterTypeRedis:
					adapter = make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), REDIS_HOST, true);
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

		number location = 0;
		for (auto _ : state)
		{
			if (batch == 1)
			{
				adapter->set((location * (1 << 10)) % CAPACITY, {5uLL, bytes()});
			}
			else
			{
				vector<pair<number, pair<number, bytes>>> writes;
				writes.resize(batch);
				for (auto i = 0; i < batch; i++)
				{
					writes[i] = {((location + i) * (1 << 10)) % CAPACITY, {5uLL, bytes()}};
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

		for (number i = 0; i < CAPACITY; i++)
		{
			adapter->set(i, {5uLL, bytes()});
		}

		number location = 0;
		for (auto _ : state)
		{
			if (batch == 1)
			{
				benchmark::DoNotOptimize(adapter->get((location * (1 << 10)) % CAPACITY));
			}
			else
			{
				vector<number> reads;
				reads.resize(batch);
				for (auto i = 0; i < batch; i++)
				{
					reads[i] = ((location + i) * (1 << 10)) % CAPACITY;
				}
				benchmark::DoNotOptimize(adapter->get(reads));
			}
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

		try
		{
			// test if Redis is availbale
			make_unique<sw::redis::Redis>(PathORAM::StorageAdapterBenchmark::REDIS_HOST)->ping();
			b
				->Args({StorageAdapterTypeRedis, 1})
				->Args({StorageAdapterTypeRedis, 16});
			iterations = 1 << 10;
		}
		catch (...)
		{
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
