#include "utility.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>
#include <iomanip>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <sstream>
#include <vector>

namespace PathORAM
{
	using namespace std;

	bytes getRandomBlock(number blockSize)
	{
		uchar material[blockSize];
#ifdef TESTING
		for (int i = 0; i < blockSize; i++)
		{
			material[i] = (uchar)rand();
		}
#else
		RAND_bytes(material, blockSize);
#endif
		return bytes(material, material + blockSize);
	}

	number getRandomULong(number max)
	{
		number material[1];
#ifdef TESTING
		auto intMaterial = (int *)material;
		intMaterial[0]   = rand();
		intMaterial[1]   = rand();
#else
		RAND_bytes((uchar *)material, sizeof(number));
#endif
		return material[0] % max;
	}

	uint getRandomUInt(uint max)
	{
#ifdef TESTING
		return rand() % max;
#else
		uint material[1];
		RAND_bytes((uchar *)material, sizeof(uint));
		return material[0] % max;
#endif
	}

	bytes encrypt(bytes key, bytes iv, bytes input, EncryptionMode mode)
	{
		auto size = input.size();

		if (key.size() != KEYSIZE)
		{
			throw boost::str(boost::format("key of size %1% bytes provided, need %2% bytes") % key.size() % KEYSIZE);
		}

		if (size == 0 || size % AES_BLOCK_SIZE != 0)
		{
			throw boost::str(boost::format("input must be a multiple of %1% (provided %2% bytes)") % AES_BLOCK_SIZE % size);
		}

		if (iv.size() != AES_BLOCK_SIZE)
		{
			throw boost::str(boost::format("IV of size %1% bytes provided, need %2% bytes") % iv.size() % AES_BLOCK_SIZE);
		}

		AES_KEY aesKey;
		uchar keyMaterial[KEYSIZE];
		copy(key.begin(), key.end(), keyMaterial);
		if (mode == ENCRYPT)
		{
			AES_set_encrypt_key(keyMaterial, KEYSIZE * 8, &aesKey);
		}
		else
		{
			AES_set_decrypt_key(keyMaterial, KEYSIZE * 8, &aesKey);
		}

		uchar ivMaterial[AES_BLOCK_SIZE];
		copy(iv.begin(), iv.end(), ivMaterial);

		uchar inputMaterial[size];
		copy(input.begin(), input.end(), inputMaterial);

		uchar outputMaterial[size];
		memset(outputMaterial, 0x00, size);

		AES_cbc_encrypt((const uchar *)inputMaterial, outputMaterial, size, &aesKey, ivMaterial, mode == ENCRYPT ? AES_ENCRYPT : AES_DECRYPT);

		return bytes(outputMaterial, outputMaterial + size);
	}

	bytes fromText(string text, number BLOCK_SIZE)
	{
		stringstream padded;
		padded << setw(BLOCK_SIZE - 1) << left << text << endl;
		text = padded.str();

		return bytes((uchar *)text.c_str(), (uchar *)text.c_str() + text.length());
	}

	string toText(bytes data, number BLOCK_SIZE)
	{
		char buffer[BLOCK_SIZE];
		memset(buffer, 0, sizeof buffer);
		copy(data.begin(), data.end(), buffer);
		buffer[BLOCK_SIZE - 1] = '\0';
		auto text			   = string(buffer);
		boost::algorithm::trim_right(text);
		return text;
	}
}
