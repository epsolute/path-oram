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
		inline static number LOG_CAPACITY;
		inline static number Z;
		inline static number BLOCK_SIZE;
		inline static number BATCH_SIZE;

		inline static number CAPACITY;
		inline static number ELEMENTS;

		inline static const auto ITERATIONS = 1 << 10;

		protected:
		unique_ptr<ORAM> oram;

		void Configure(number LOG_CAPACITY, number Z, number BLOCK_SIZE, number BATCH_SIZE)
		{
			this->LOG_CAPACITY = LOG_CAPACITY;
			this->Z			   = Z;
			this->BLOCK_SIZE   = BLOCK_SIZE;
			this->BATCH_SIZE   = BATCH_SIZE;
			this->CAPACITY	   = (1 << LOG_CAPACITY) * Z;
			this->ELEMENTS	   = (CAPACITY / 4) * 3;

			this->oram = make_unique<ORAM>(
				LOG_CAPACITY,
				BLOCK_SIZE,
				Z,
				make_unique<InMemoryStorageAdapter>(CAPACITY + Z, BLOCK_SIZE, bytes(), Z),
				make_unique<InMemoryPositionMapAdapter>(CAPACITY + Z),
				make_unique<InMemoryStashAdapter>(3 * LOG_CAPACITY * Z),
				true,
				BATCH_SIZE);
		}
	};

	BENCHMARK_DEFINE_F(ORAMBenchmark, Payload)
	(benchmark::State& state)
	{
		Configure(state.range(0), state.range(1), state.range(2), state.range(3));

		// put all
		for (number id = 0; id < ELEMENTS; id++)
		{
			auto data = fromText(to_string(id), BLOCK_SIZE);
			oram->put(id, data);
		}

		// get all
		for (number id = 0; id < ELEMENTS; id++)
		{
			bytes returned;
			oram->get(id, returned);
		}

		// random operations
		vector<block> batch;
		auto i = 0;
		for (auto _ : state)
		{
			state.PauseTiming();

			auto id	  = getRandomULong(ELEMENTS);
			auto read = getRandomULong(2) == 0;
			auto data = fromText(to_string(ELEMENTS + getRandomULong(ELEMENTS)), BLOCK_SIZE);

			if (read)
			{
				// get
				batch.push_back({id, bytes()});
			}
			else
			{
				batch.push_back({id, data});
			}

			state.ResumeTiming();

			if (i % BATCH_SIZE == 0 || i == ITERATIONS - 1)
			{
				if (batch.size() > 0)
				{
					vector<bytes> response;
					oram->multiple(batch, response);

					batch.clear();
				}
			}
		}
	}

	BENCHMARK_REGISTER_F(ORAMBenchmark, Payload)
		// base case
		->Args({5, 3, 32, 1})

		// change Log(N)
		->Args({7, 3, 32, 1})
		->Args({9, 3, 32, 1})
		->Args({11, 3, 32, 1})

		// change Z
		->Args({5, 4, 32, 1})
		->Args({5, 5, 32, 1})
		->Args({5, 6, 32, 1})

		// change block size
		->Args({5, 3, 1024, 1})
		->Args({5, 3, 2048, 1})
		->Args({5, 3, 4096, 1})

		// change batch size
		->Args({5, 3, 32, 10})
		->Args({5, 3, 32, 25})
		->Args({5, 3, 32, 50})

		->Iterations(ORAMBenchmark::ITERATIONS)
		->Unit(benchmark::kMillisecond);
}

BENCHMARK_MAIN();
