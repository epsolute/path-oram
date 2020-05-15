#include "definitions.h"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <cmath>
#include <numeric>
#include <openssl/aes.h>

using namespace std;

namespace PathORAM
{
	class UtilityTest : public ::testing::Test
	{
	};

	TEST_F(UtilityTest, NoSeed)
	{
		const auto n = 20uLL;
		for (number i = 0; i < 100; i++)
		{
			auto first	= getRandomBlock(n);
			auto second = getRandomBlock(n);

			ASSERT_NE(first, second);
		}
	}

	TEST_F(UtilityTest, SameSeed)
	{
		const auto n = 20uLL;
		int seed	 = 0x15;

		srand(seed);

		auto first = getRandomBlock(n);

		srand(seed);

		auto second = getRandomBlock(n);

		ASSERT_EQ(first, second);
	}

	TEST_F(UtilityTest, RandomDoubleBasicTest)
	{
		const auto n   = 10000uLL;
		const auto max = 10.0;
		double total   = 0;

		for (auto i = 0uLL; i < n; i++)
		{
			auto sample = getRandomDouble(max);
			EXPECT_LT(sample, max);
			EXPECT_GE(sample, 0);

			total += sample;
		}

		EXPECT_NEAR(max / 2, total / n, 0.05);
	}

	TEST_F(UtilityTest, EncryptionInputChecks)
	{
		for (auto mode : {ENCRYPT, DECRYPT})
		{
			ASSERT_ANY_THROW(encrypt(getRandomBlock(KEYSIZE - 1), getRandomBlock(AES_BLOCK_SIZE), getRandomBlock(3 * AES_BLOCK_SIZE), mode));
			ASSERT_ANY_THROW(encrypt(getRandomBlock(KEYSIZE), getRandomBlock(AES_BLOCK_SIZE - 1), getRandomBlock(3 * AES_BLOCK_SIZE), mode));
			ASSERT_ANY_THROW(encrypt(getRandomBlock(KEYSIZE), getRandomBlock(AES_BLOCK_SIZE), getRandomBlock(3 * AES_BLOCK_SIZE - 1), mode));
		}
	}

	TEST_F(UtilityTest, EncryptDecryptSingle)
	{
		auto key = getRandomBlock(KEYSIZE);
		auto iv	 = getRandomBlock(AES_BLOCK_SIZE);

		auto input = fromText("Hello, world!", 64);

		auto ciphertext = encrypt(key, iv, input, ENCRYPT);

		auto plaintext = encrypt(key, iv, ciphertext, DECRYPT);

		ASSERT_EQ(input, plaintext);
	}

	TEST_F(UtilityTest, EncryptDecryptManyCBC)
	{
		for (number i = 0; i < 100; i++)
		{
			auto key   = getRandomBlock(KEYSIZE);
			auto iv	   = getRandomBlock(AES_BLOCK_SIZE);
			auto input = getRandomBlock(AES_BLOCK_SIZE * 3);

			auto ciphertext = encrypt(key, iv, input, ENCRYPT);

			auto plaintext = encrypt(key, iv, ciphertext, DECRYPT);

			ASSERT_EQ(input, plaintext);
		}
	}

	TEST_F(UtilityTest, EncryptDecryptManyCTR)
	{
		__blockCipherMode = CTR;

		for (number i = 0; i < 100; i++)
		{
			auto key   = getRandomBlock(KEYSIZE);
			auto iv	   = getRandomBlock(AES_BLOCK_SIZE);
			auto input = getRandomBlock(AES_BLOCK_SIZE * 3);

			auto ciphertext = encrypt(key, iv, input, ENCRYPT);

			auto plaintext = encrypt(key, iv, ciphertext, DECRYPT);

			ASSERT_EQ(input, plaintext);
		}
	}

	TEST_F(UtilityTest, UnimplementedMode)
	{
		__blockCipherMode = (BlockCipherMode)INT_MAX;

		ASSERT_ANY_THROW(
			encrypt(
				getRandomBlock(KEYSIZE),
				getRandomBlock(AES_BLOCK_SIZE),
				getRandomBlock(AES_BLOCK_SIZE * 3),
				ENCRYPT));
	}

	TEST_F(UtilityTest, LoadStoreKey)
	{
		auto key = getRandomBlock(KEYSIZE);
		storeKey(key, "key.bin");
		auto loaded = loadKey("key.bin");
		EXPECT_EQ(key, loaded);
		remove("key.bin");
	}

	TEST_F(UtilityTest, LoadStoreKeyFileErrors)
	{
		ASSERT_ANY_THROW(storeKey(bytes(), "/error/path/should/not/exist"));
		ASSERT_ANY_THROW(loadKey("/error/path/should/not/exist"));
	}

	TEST_F(UtilityTest, HashSameInput)
	{
		auto input	= fromText("Hello, world", 500);
		auto first	= hash(input);
		auto second = hash(input);

		ASSERT_EQ(first, second);
	}

	TEST_F(UtilityTest, HashDifferentInput)
	{
		auto first	= hash(fromText("Hello, world", 500));
		auto second = hash(fromText("Hi", 500));

		ASSERT_NE(first, second);
	}

	TEST_F(UtilityTest, HashExpectedSize)
	{
		auto disgest = hash(fromText("Hello, world", 500));

		ASSERT_EQ(HASHSIZE / 16, disgest.size());
	}

	TEST_F(UtilityTest, HashToNumberUniform)
	{
		const auto RUNS = 10000uLL;
		const auto MAX	= 10uLL;
		vector<number> bins(MAX, 0);

		for (auto i = 0uLL; i < RUNS; i++)
		{
			number material[1] = {i};
			auto input		   = bytes((uchar *)material, (uchar *)material + sizeof(number));

			auto sample = hashToNumber(input, MAX);
			EXPECT_LT(sample, MAX);
			EXPECT_GE(sample, 0);

			bins[sample]++;
		}

		auto sum  = accumulate(bins.begin(), bins.end(), 0.0);
		auto mean = sum / bins.size();

		auto sq_sum = inner_product(bins.begin(), bins.end(), bins.begin(), 0.0);
		auto stddev = sqrt(sq_sum / bins.size() - mean * mean);

		EXPECT_NEAR(RUNS / (double)MAX, mean, 0.01);
		EXPECT_NEAR(0.0, stddev, 0.01 * RUNS);
	}
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
