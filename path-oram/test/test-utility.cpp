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
			ASSERT_ANY_THROW({
				auto key   = getRandomBlock(KEYSIZE - 1);
				auto iv	   = getRandomBlock(AES_BLOCK_SIZE);
				auto input = getRandomBlock(3 * AES_BLOCK_SIZE);
				bytes output;
				encrypt(
					key.begin(),
					key.end(),
					iv.begin(),
					iv.end(),
					input.begin(),
					input.end(),
					output,
					mode);
			});
			ASSERT_ANY_THROW({
				auto key   = getRandomBlock(KEYSIZE);
				auto iv	   = getRandomBlock(AES_BLOCK_SIZE - 1);
				auto input = getRandomBlock(3 * AES_BLOCK_SIZE);
				bytes output;
				encrypt(
					key.begin(),
					key.end(),
					iv.begin(),
					iv.end(),
					input.begin(),
					input.end(),
					output,
					mode);
			});
			ASSERT_ANY_THROW({
				auto key   = getRandomBlock(KEYSIZE);
				auto iv	   = getRandomBlock(AES_BLOCK_SIZE);
				auto input = getRandomBlock(3 * AES_BLOCK_SIZE - 1);
				bytes output;
				encrypt(
					key.begin(),
					key.end(),
					iv.begin(),
					iv.end(),
					input.begin(),
					input.end(),
					output,
					mode);
			});
		}
	}

	TEST_F(UtilityTest, EncryptDecryptSingle)
	{
		auto key = getRandomBlock(KEYSIZE);
		auto iv	 = getRandomBlock(AES_BLOCK_SIZE);

		auto input = fromText("Hello, world!", 64);

		bytes ciphertext;
		encrypt(
			key.begin(),
			key.end(),
			iv.begin(),
			iv.end(),
			input.begin(),
			input.end(),
			ciphertext,
			ENCRYPT);

		bytes plaintext;
		encrypt(
			key.begin(),
			key.end(),
			iv.begin(),
			iv.end(),
			ciphertext.begin(),
			ciphertext.end(),
			plaintext,
			DECRYPT);

		ASSERT_EQ(input, plaintext);
	}

	TEST_F(UtilityTest, EncryptDecryptManyCBC)
	{
		for (number i = 0; i < 100; i++)
		{
			auto key   = getRandomBlock(KEYSIZE);
			auto iv	   = getRandomBlock(AES_BLOCK_SIZE);
			auto input = getRandomBlock(AES_BLOCK_SIZE * 3);

			bytes ciphertext;
			encrypt(
				key.begin(),
				key.end(),
				iv.begin(),
				iv.end(),
				input.begin(),
				input.end(),
				ciphertext,
				ENCRYPT);

			bytes plaintext;
			encrypt(
				key.begin(),
				key.end(),
				iv.begin(),
				iv.end(),
				ciphertext.begin(),
				ciphertext.end(),
				plaintext,
				DECRYPT);

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

			bytes ciphertext;
			encrypt(
				key.begin(),
				key.end(),
				iv.begin(),
				iv.end(),
				input.begin(),
				input.end(),
				ciphertext,
				ENCRYPT);

			bytes plaintext;
			encrypt(
				key.begin(),
				key.end(),
				iv.begin(),
				iv.end(),
				ciphertext.begin(),
				ciphertext.end(),
				plaintext,
				DECRYPT);

			ASSERT_EQ(input, plaintext);
		}
	}

	TEST_F(UtilityTest, EncryptDecryptNoEncryption)
	{
		__blockCipherMode = NONE;

		auto key   = getRandomBlock(KEYSIZE);
		auto iv	   = getRandomBlock(AES_BLOCK_SIZE);
		auto input = getRandomBlock(3 * AES_BLOCK_SIZE);
		bytes output;
		encrypt(
			key.begin(),
			key.end(),
			iv.begin(),
			iv.end(),
			input.begin(),
			input.end(),
			output,
			ENCRYPT);

		ASSERT_EQ(input, output);

		output.clear();
		encrypt(
			key.begin(),
			key.end(),
			iv.begin(),
			iv.end(),
			input.begin(),
			input.end(),
			output,
			DECRYPT);

		ASSERT_EQ(input, output);
	}

	TEST_F(UtilityTest, UnimplementedMode)
	{
		__blockCipherMode = (BlockCipherMode)INT_MAX;

		ASSERT_ANY_THROW({
			auto key   = getRandomBlock(KEYSIZE);
			auto iv	   = getRandomBlock(AES_BLOCK_SIZE);
			auto input = getRandomBlock(3 * AES_BLOCK_SIZE);
			bytes output;
			encrypt(
				key.begin(),
				key.end(),
				iv.begin(),
				iv.end(),
				input.begin(),
				input.end(),
				output,
				ENCRYPT);
		});
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
		auto input = fromText("Hello, world", 500);
		bytes first, second;
		hash(input, first);
		hash(input, second);

		ASSERT_EQ(first, second);
	}

	TEST_F(UtilityTest, HashDifferentInput)
	{
		auto inputOne = fromText("Hello, world", 500);
		auto inputTwo = fromText("Hi", 500);
		bytes first, second;
		hash(inputOne, first);
		hash(inputTwo, second);

		ASSERT_NE(first, second);
	}

	TEST_F(UtilityTest, HashExpectedSize)
	{
		auto input = fromText("Hello, world", 500);
		bytes digest;
		hash(input, digest);

		ASSERT_EQ(HASHSIZE / 16, digest.size());
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
