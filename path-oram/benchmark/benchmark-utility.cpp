#include "definitions.h"
#include "utility.hpp"

#include <benchmark/benchmark.h>
#include <openssl/aes.h>

using namespace std;

namespace PathORAM
{
	class UtilityBenchmark : public ::benchmark::Fixture
	{
	};

	BENCHMARK_DEFINE_F(UtilityBenchmark, Random)
	(benchmark::State& state)
	{
		for (auto _ : state)
		{
			benchmark::DoNotOptimize(getRandomBlock(64));
		}
	}

	BENCHMARK_DEFINE_F(UtilityBenchmark, Encrypt)
	(benchmark::State& state)
	{
		auto palintext = getRandomBlock(64);
		auto key	   = getRandomBlock(KEYSIZE);
		auto iv		   = getRandomBlock(AES_BLOCK_SIZE);

		for (auto _ : state)
		{
			benchmark::DoNotOptimize(encrypt(key, iv, palintext, ENCRYPT));
		}
	}

	BENCHMARK_DEFINE_F(UtilityBenchmark, Decrypt)
	(benchmark::State& state)
	{
		auto palintext  = getRandomBlock(64);
		auto key		= getRandomBlock(KEYSIZE);
		auto iv			= getRandomBlock(AES_BLOCK_SIZE);
		auto ciphertext = encrypt(key, iv, palintext, ENCRYPT);

		for (auto _ : state)
		{
			benchmark::DoNotOptimize(encrypt(key, iv, ciphertext, DECRYPT));
		}
	}

	BENCHMARK_REGISTER_F(UtilityBenchmark, Random)
		->Iterations(1 << 20)
		->Unit(benchmark::kMicrosecond);

	BENCHMARK_REGISTER_F(UtilityBenchmark, Encrypt)
		->Iterations(1 << 20)
		->Unit(benchmark::kMicrosecond);

	BENCHMARK_REGISTER_F(UtilityBenchmark, Decrypt)
		->Iterations(1 << 20)
		->Unit(benchmark::kMicrosecond);
}

BENCHMARK_MAIN();
