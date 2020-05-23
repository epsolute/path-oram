#pragma once

#include "definitions.h"

#include <string>

namespace PathORAM
{
	using namespace std;

	/**
	 * @brief generate an array of bytes pseudorandomly
	 *
	 * \note
	 * It uses OpenSSL PRG unless TESTING macro is defined.
	 * If it is, C++ standard rand() is used (easy for testing and debugging).
	 *
	 * @param blockSize the number of bytes to generate
	 * @return bytes the resulting bytes
	 */
	bytes getRandomBlock(number blockSize);

	/**
	 * @brief returns a pseudorandom number
	 *
	 * @param max the non-inclusive max of the range (min is inclusive 0).
	 * @return number the resulting number
	 */
	number getRandomULong(number max);

	/**
	 * @brief returns a pseudorandom double
	 *
	 * @param max the non-inclusive max of the range (min is inclusive 0).
	 * @return double the resulting double
	 */
	double getRandomDouble(double max);

	/**
	 * @brief returns a pseudorandom uint
	 *
	 * A faster and smaller scale version of getRandomULong
	 *
	 * @param max the non-inclusive max of the range (min is inclusive 0).
	 * @return uint the resulting int
	 */
	uint getRandomUInt(uint max);

	/**
	 * @brief Encryption routine
	 *
	 * Does encryption or decryption using OpenSSL AES-MODE-256 (or whatver size key is provided, defined by KEYSIZE).
	 * The MODE is configured with global setting __blockCipherMode (currently CBC or CTR).
	 *
	 * @param key the AES key (must be KEYSIZE bytes)
	 * @param iv initialization vector (better be randomly generated, must be of size of the AES block, 16 bytes)
	 * @param input the plaintext or ciphertext material, without IV, must be the multiple of AES block size (16 bytes)
	 * @param mode ENCRYPTION or DECRYPTION
	 * @return bytes the ciphertex or plaintex material, the result of the encryption operation
	 */
	void encrypt(
		bytes::const_iterator keyFirst,
		bytes::const_iterator keyLast,
		bytes::const_iterator ivFist,
		bytes::const_iterator ivLast,
		bytes::const_iterator inputFist,
		bytes::const_iterator inputLast,
		bytes &output,
		EncryptionMode mode);

	/**
	 * @brief helper to convert string to bytes and pad (from right with zeros)
	 *
	 * @param text the string to convert
	 * @param BLOCK_SIZE the size of the resulting byte vector (will be right-padded with zeros)
	 * @return bytes the padded bytes
	 */
	bytes fromText(string text, number BLOCK_SIZE);

	/**
	 * @brief helper to convert the bytes to string (counterpart of fromText)
	 *
	 * @param data bytes produced with fromText
	 * @param BLOCK_SIZE same as in fromText
	 * @return string the original string supplied to fromText
	 */
	string toText(bytes data, number BLOCK_SIZE);

	/**
	 * @brief write key to a binary file
	 *
	 * @param key the key to write
	 * @param filename the name of the file to write to
	 */
	void storeKey(bytes key, string filename);

	/**
	 * @brief read key from a binary file
	 *
	 * @param filename the name of the file to read from
	 * @return bytes the key read (KEYSIZE length)
	 */
	bytes loadKey(string filename);

	/**
	 * @brief compute a hash of the given input using OpenSSL algorithms.
	 * SHA-2 family is used, disgest size in bits is HASHSIZE macro (currently 256).
	 *
	 * @param input the message to disgest
	 * @return bytes the digest of the message
	 */
	bytes hash(bytes input);

	/**
	 * @brief compute a hash of the message in a form of a number from zero to max.
	 * Simply computes a SHA2 hash, takes first sizeof(number) bytes and returns this mod max.
	 *
	 * @param input the message to disgest
	 * @param max the exclusive max number of the output
	 * @return number the hash of the message as a number [0, max)
	 */
	number hashToNumber(bytes input, number max);
}
