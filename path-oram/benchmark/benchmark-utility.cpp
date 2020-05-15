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

	BENCHMARK_DEFINE_F(UtilityBenchmark, Hash)
	(benchmark::State& state)
	{
		auto i = 0uLL;
		for (auto _ : state)
		{
			number material[1] = {i};
			auto input		   = bytes((uchar*)material, (uchar*)material + sizeof(number));
			benchmark::DoNotOptimize(hash(input));
		}
	}

	BENCHMARK_DEFINE_F(UtilityBenchmark, Encrypt)
	(benchmark::State& state)
	{
		__blockCipherMode = (BlockCipherMode)state.range(0);

		auto palintext = getRandomBlock(1024);
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
		__blockCipherMode = (BlockCipherMode)state.range(0);

		auto palintext	= getRandomBlock(1024);
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

	BENCHMARK_REGISTER_F(UtilityBenchmark, Hash)
		->Iterations(1 << 20)
		->Unit(benchmark::kMicrosecond);

	BENCHMARK_REGISTER_F(UtilityBenchmark, Encrypt)
		->Args({CBC})
		->Args({CTR})
		->Iterations(1 << 15)
		->Unit(benchmark::kMicrosecond);

	BENCHMARK_REGISTER_F(UtilityBenchmark, Decrypt)
		->Args({CBC})
		->Args({CTR})
		->Iterations(1 << 15)
		->Unit(benchmark::kMicrosecond);
}

BENCHMARK_MAIN();
