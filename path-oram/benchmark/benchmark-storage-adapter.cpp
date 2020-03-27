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
		StorageAdapterTypeFileSystem
	};

	class StorageAdapterBenchmark : public ::benchmark::Fixture
	{
		public:
		inline static const ulong CAPACITY   = 1 << 17;
		inline static const ulong BLOCK_SIZE = 32;
		inline static const string FILE_NAME = "storage.bin";

		protected:
		AbsStorageAdapter* adapter;

		~StorageAdapterBenchmark() override
		{
			delete adapter;
			remove(FILE_NAME.c_str());
		}

		void Configure(TestingStorageAdapterType type)
		{
			switch (type)
			{
				case StorageAdapterTypeInMemory:
					adapter = new InMemoryStorageAdapter(CAPACITY, BLOCK_SIZE, bytes());
					break;
				case StorageAdapterTypeFileSystem:
					adapter = new FileSystemStorageAdapter(CAPACITY, BLOCK_SIZE, bytes(), FILE_NAME, true);
					break;
				default:
					throw boost::str(boost::format("TestingStorageAdapterType %2% is not implemented") % type);
			}
		}
	};

	BENCHMARK_DEFINE_F(StorageAdapterBenchmark, Write)
	(benchmark::State& state)
	{
		Configure((TestingStorageAdapterType)state.range(0));
		auto random = state.range(1);

		ulong location = 0;
		for (auto _ : state)
		{
			adapter->set((location * (random ? (1 << 10) : 1)) % CAPACITY, {5uLL, bytes()});
			location++;
		}
	}

	BENCHMARK_DEFINE_F(StorageAdapterBenchmark, Read)
	(benchmark::State& state)
	{
		Configure((TestingStorageAdapterType)state.range(0));
		auto random = state.range(1);

		for (ulong i = 0; i < CAPACITY; i++)
		{
			adapter->set(i, {5uLL, bytes()});
		}

		ulong location = 0;
		for (auto _ : state)
		{
			benchmark::DoNotOptimize(adapter->get((location * (random ? (1 << 10) : 1)) % CAPACITY));
		}
	}

	BENCHMARK_REGISTER_F(StorageAdapterBenchmark, Write)
		->Args({StorageAdapterTypeInMemory, false})
		->Args({StorageAdapterTypeInMemory, true})
		->Args({StorageAdapterTypeFileSystem, false})
		->Args({StorageAdapterTypeFileSystem, true})
		->Iterations(1 << 15)
		->Unit(benchmark::kMicrosecond);

	BENCHMARK_REGISTER_F(StorageAdapterBenchmark, Read)
		->Args({StorageAdapterTypeInMemory, false})
		->Args({StorageAdapterTypeInMemory, true})
		->Args({StorageAdapterTypeFileSystem, false})
		->Args({StorageAdapterTypeFileSystem, true})
		->Iterations(1 << 15)
		->Unit(benchmark::kMicrosecond);
}

BENCHMARK_MAIN();
