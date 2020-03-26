#include "definitions.h"
#include "oram.hpp"
#include "utility.hpp"

#include <benchmark/benchmark.h>
#include <boost/format.hpp>
#include <fstream>

using namespace std;

namespace PathORAM
{
	class ORAMBenchmark : public ::benchmark::Fixture
	{
		public:
		inline static ulong LOG_CAPACITY;
		inline static ulong Z;
		inline static ulong BLOCK_SIZE;

		inline static ulong CAPACITY;
		inline static ulong ELEMENTS;

		protected:
		ORAM* oram;
		AbsStorageAdapter* storage;
		AbsPositionMapAdapter* map;
		AbsStashAdapter* stash;

		void Configure(ulong LOG_CAPACITY, ulong Z, ulong BLOCK_SIZE)
		{
			this->LOG_CAPACITY = LOG_CAPACITY;
			this->Z			   = Z;
			this->BLOCK_SIZE   = BLOCK_SIZE;
			this->CAPACITY	 = (1 << LOG_CAPACITY) * Z;
			this->ELEMENTS	 = (CAPACITY / 4) * 3;

			this->storage = new InMemoryStorageAdapter(CAPACITY + Z, BLOCK_SIZE, bytes());
			this->map	 = new InMemoryPositionMapAdapter(CAPACITY + Z);
			this->stash   = new InMemoryStashAdapter(3 * LOG_CAPACITY * Z);

			this->oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, this->storage, this->map, this->stash);
		}

		~ORAMBenchmark() override
		{
			delete oram;
			delete storage;
			delete map;
			delete stash;
		}
	};

	BENCHMARK_DEFINE_F(ORAMBenchmark, Payload)
	(benchmark::State& state)
	{
		Configure(state.range(0), state.range(1), state.range(2));

		// put all
		for (ulong id = 0; id < ELEMENTS; id++)
		{
			auto data = fromText(to_string(id), BLOCK_SIZE);
			this->oram->put(id, data);
		}

		// get all
		for (ulong id = 0; id < ELEMENTS; id++)
		{
			this->oram->get(id);
		}

		// random operations
		for (auto _ : state)
		{
			auto id   = getRandomULong(ELEMENTS);
			auto read = getRandomULong(2) == 0;
			if (read)
			{
				// get
				benchmark::DoNotOptimize(this->oram->get(id));
			}
			else
			{
				auto data = fromText(to_string(ELEMENTS + getRandomULong(ELEMENTS)), BLOCK_SIZE);
				this->oram->put(id, data);
			}
		}
	}

	BENCHMARK_REGISTER_F(ORAMBenchmark, Payload)
		->Args({5, 3, 32})

		->Args({7, 3, 32})
		->Args({9, 3, 32})
		->Args({11, 3, 32})

		->Args({5, 4, 32})
		->Args({5, 5, 32})
		->Args({5, 6, 32})

		->Args({5, 3, 1024})
		->Args({5, 3, 2048})
		->Args({5, 3, 4096})

		->Iterations(1 << 10)
		->Unit(benchmark::kMillisecond);
}

BENCHMARK_MAIN();