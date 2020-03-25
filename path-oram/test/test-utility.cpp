#include "definitions.h"
#include "utility.hpp"

#include "gtest/gtest.h"
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
		for (ulong i = 0; i < 100; i++)
		{
			auto first  = getRandomBlock(n);
			auto second = getRandomBlock(n);

			ASSERT_NE(first, second);
		}
	}

	TEST_F(UtilityTest, SameSeed)
	{
		const auto n = 20uLL;
		int seed	 = 0x13;

		seedRandom(seed);

		auto first = getRandomBlock(n);

		seedRandom(seed);

		auto second = getRandomBlock(n);

		ASSERT_EQ(first, second);
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
		auto iv  = getRandomBlock(AES_BLOCK_SIZE);

		auto input = fromText("Hello, world!", 64);

		auto ciphertext = encrypt(key, iv, input, ENCRYPT);

		auto plaintext = encrypt(key, iv, ciphertext, DECRYPT);

		ASSERT_EQ(input, plaintext);
	}

	TEST_F(UtilityTest, EncryptDecryptMany)
	{
		for (ulong i = 0; i < 100; i++)
		{
			auto key   = getRandomBlock(KEYSIZE);
			auto iv	= getRandomBlock(AES_BLOCK_SIZE);
			auto input = getRandomBlock(AES_BLOCK_SIZE * 3);

			auto ciphertext = encrypt(key, iv, input, ENCRYPT);

			auto plaintext = encrypt(key, iv, ciphertext, DECRYPT);

			ASSERT_EQ(input, plaintext);
		}
	}
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
