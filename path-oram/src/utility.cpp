#include "utility.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <iomanip>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/modes.h>
#include <openssl/rand.h>
#include <random>
#include <sstream>
#include <vector>

#define HANDLE_ERROR(statement)      \
	if (!(statement))                \
	{                                \
		throw Exception(#statement); \
	}

namespace PathORAM
{
	using namespace std;

	bytes getRandomBlock(const number blockSize)
	{
		uchar material[blockSize];
#if defined(TESTING) || defined(DEBUG)
		for (number i = 0; i < blockSize; i++)
		{
			material[i] = (uchar)rand();
		}
#else
		RAND_bytes(material, blockSize);
#endif
		return bytes(material, material + blockSize);
	}

	number getRandomULong(const number max)
	{
		number material[1];
#if defined(TESTING) || defined(DEBUG)
		auto intMaterial = (int *)material;
		intMaterial[0]	 = rand();
		intMaterial[1]	 = rand();
#else
		RAND_bytes((uchar *)material, sizeof(number));
#endif
		return material[0] % max;
	}

	uint getRandomUInt(const uint max)
	{
#if defined(TESTING) || defined(DEBUG)
		return rand() % max;
#else
		uint material[1];
		RAND_bytes((uchar *)material, sizeof(uint));
		return material[0] % max;
#endif
	}

	double getRandomDouble(const double max)
	{
		number material[1];
#if defined(TESTING) || defined(DEBUG)
		auto intMaterial = (int *)material;
		intMaterial[0]	 = rand();
		intMaterial[1]	 = rand();
#else
		RAND_bytes((uchar *)material, sizeof(number));
#endif
		mt19937_64 gen(material[0]);
		uniform_real_distribution<> distribution(0, max);

		return distribution(gen);
	}

	void encrypt(
		const bytes::const_iterator keyFirst,
		const bytes::const_iterator keyLast,
		const bytes::const_iterator ivFist,
		const bytes::const_iterator ivLast,
		const bytes::const_iterator inputFirst,
		const bytes::const_iterator inputLast,
		bytes &output,
		const EncryptionMode mode)
	{
		const auto size = distance(inputFirst, inputLast);

#if INPUT_CHECKS
		if (distance(keyFirst, keyLast) != KEYSIZE)
		{
			throw Exception(boost::format("key of size %1% bytes provided, need %2% bytes") % distance(keyFirst, keyLast) % KEYSIZE);
		}

		if (size == 0 || size % AES_BLOCK_SIZE != 0)
		{
			throw Exception(boost::format("input must be a multiple of %1% (provided %2% bytes)") % AES_BLOCK_SIZE % size);
		}

		if (distance(ivFist, ivLast) != AES_BLOCK_SIZE)
		{
			throw Exception(boost::format("IV of size %1% bytes provided, need %2% bytes") % distance(ivFist, ivLast) % AES_BLOCK_SIZE);
		}
#endif

		if (__blockCipherMode == NONE)
		{
			output.insert(output.end(), inputFirst, inputLast);
			return;
		}

		AES_KEY aesKey;
		uchar keyMaterial[KEYSIZE];
		copy(keyFirst, keyLast, keyMaterial);
		// CTR always does encryption only
		if (mode == ENCRYPT || __blockCipherMode == CTR)
		{
			AES_set_encrypt_key(keyMaterial, KEYSIZE * 8, &aesKey);
		}
		else
		{
			AES_set_decrypt_key(keyMaterial, KEYSIZE * 8, &aesKey);
		}

		uchar ivMaterial[AES_BLOCK_SIZE];
		copy(ivFist, ivLast, ivMaterial);

		uchar inputMaterial[size];
		copy(inputFirst, inputLast, inputMaterial);

		uchar outputMaterial[size];
		memset(outputMaterial, 0x00, size);

		uint ctr_num = 0;
		uchar ctr_buffer[AES_BLOCK_SIZE];
		memset(ctr_buffer, 0x00, AES_BLOCK_SIZE);

		// this last parameter is essentially a block cipher itself (as a function);
		// CRYPTO_ family functions are cipher-agnostic
		switch (__blockCipherMode)
		{
			case CBC:
				(mode == ENCRYPT ? CRYPTO_cbc128_encrypt : CRYPTO_cbc128_decrypt)(
					(const uchar *)inputMaterial,
					outputMaterial,
					size,
					&aesKey,
					ivMaterial,
					(block128_f)(mode == ENCRYPT ? AES_encrypt : AES_decrypt));
				break;

			case CTR:
				CRYPTO_ctr128_encrypt(
					(const uchar *)inputMaterial,
					outputMaterial,
					size,
					&aesKey,
					ivMaterial,
					ctr_buffer,
					&ctr_num,
					(block128_f)AES_encrypt);
				break;

			default:
				throw Exception(boost::format("Block cipher mode not implemented: %1%") % __blockCipherMode);
		}

		output.insert(output.end(), outputMaterial, outputMaterial + size);
	}

	bytes fromText(const string text, const number BLOCK_SIZE)
	{
		stringstream padded;
		padded << setw(BLOCK_SIZE - 1) << left << text << endl;
		auto paddedStr = padded.str();

		return bytes((uchar *)paddedStr.c_str(), (uchar *)paddedStr.c_str() + paddedStr.length());
	}

	string toText(const bytes data, const number BLOCK_SIZE)
	{
		char buffer[BLOCK_SIZE];
		memset(buffer, 0, sizeof buffer);
		copy(data.begin(), data.end(), buffer);
		buffer[BLOCK_SIZE - 1] = '\0';
		auto text			   = string(buffer);
		boost::algorithm::trim_right(text);
		return text;
	}

	void storeKey(const bytes key, const string filename)
	{
		fstream file;
		file.open(filename, fstream::out | fstream::binary | fstream::trunc);
		if (!file)
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		uchar material[KEYSIZE];
		copy(key.begin(), key.end(), material);
		file.seekg(0, file.beg);
		file.write((const char *)material, KEYSIZE);
		file.close();
	}

	bytes loadKey(const string filename)
	{
		fstream file;
		file.open(filename, fstream::in | fstream::binary);
		if (!file)
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		uchar material[KEYSIZE];
		file.seekg(0, file.beg);
		file.read((char *)material, KEYSIZE);
		file.close();

		return bytes(material, material + KEYSIZE);
	}

	void hash(const bytes &input, bytes &output)
	{
		// https: //wiki.openssl.org/index.php/EVP_Message_Digests

		uchar message[input.size()];
		uchar **digest = (uchar **)malloc(HASHSIZE / 16);
		copy(input.begin(), input.end(), message);

		EVP_MD_CTX *context;
		HANDLE_ERROR((context = EVP_MD_CTX_new()) != nullptr);
		HANDLE_ERROR(EVP_DigestInit_ex(context, HASH_ALGORITHM(), nullptr));
		HANDLE_ERROR(EVP_DigestUpdate(context, message, input.size()));
		HANDLE_ERROR((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(HASH_ALGORITHM()))) != nullptr);
		HANDLE_ERROR(EVP_DigestFinal_ex(context, *digest, nullptr));

		output.insert(output.end(), *digest, *digest + (HASHSIZE / 16));

		EVP_MD_CTX_free(context);
		free(digest);
	}

	number hashToNumber(const bytes &input, number max)
	{
		bytes digest;
		hash(input, digest);
		number material[1];
		auto ucharMaterial = (uchar *)material;

		copy(digest.begin(), digest.begin() + sizeof(number), ucharMaterial);

		return material[0] % max;
	}
}
