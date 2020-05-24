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
	bytes getRandomBlock(const number blockSize);

	/**
	 * @brief returns a pseudorandom number
	 *
	 * @param max the non-inclusive max of the range (min is inclusive 0).
	 * @return number the resulting number
	 */
	number getRandomULong(const number max);

	/**
	 * @brief returns a pseudorandom double
	 *
	 * @param max the non-inclusive max of the range (min is inclusive 0).
	 * @return double the resulting double
	 */
	double getRandomDouble(const double max);

	/**
	 * @brief returns a pseudorandom uint
	 *
	 * A faster and smaller scale version of getRandomULong
	 *
	 * @param max the non-inclusive max of the range (min is inclusive 0).
	 * @return uint the resulting int
	 */
	uint getRandomUInt(const uint max);

	/**
	 * @brief Encryption routine
	 *
	 * Does encryption or decryption using OpenSSL AES-MODE-256 (or whatver size key is provided, defined by KEYSIZE).
	 * The MODE is configured with global setting __blockCipherMode (currently CBC or CTR).
	 *
	 * @param keyFirst the begin() iterator of AES key (must be KEYSIZE bytes)
	 * @param keyLast the end() iterator of AES key (must be KEYSIZE bytes)
	 * @param ivFirst the begin() iterator of initialization vector (better be randomly generated, must be of size of the AES block, 16 bytes)
	 * @param ivLast the end() iterator of initialization vector (better be randomly generated, must be of size of the AES block, 16 bytes)
	 * @param inputFirst the begin() iterator of the plaintext or ciphertext material, without IV, must be the multiple of AES block size (16 bytes)
	 * @param inputLast the end() iterator of the plaintext or ciphertext material, without IV, must be the multiple of AES block size (16 bytes)
	 * @param the vector to put the ciphertext or plaintex material, the result of the encryption operation
	 * @param mode ENCRYPTION or DECRYPTION
	 */
	void encrypt(
		const bytes::const_iterator keyFirst,
		const bytes::const_iterator keyLast,
		const bytes::const_iterator ivFist,
		const bytes::const_iterator ivLast,
		const bytes::const_iterator inputFist,
		const bytes::const_iterator inputLast,
		bytes &output,
		const EncryptionMode mode);

	/**
	 * @brief helper to convert string to bytes and pad (from right with zeros)
	 *
	 * @param text the string to convert
	 * @param BLOCK_SIZE the size of the resulting byte vector (will be right-padded with zeros)
	 * @return bytes the padded bytes
	 */
	bytes fromText(const string text, const number BLOCK_SIZE);

	/**
	 * @brief helper to convert the bytes to string (counterpart of fromText)
	 *
	 * @param data bytes produced with fromText
	 * @param BLOCK_SIZE same as in fromText
	 * @return string the original string supplied to fromText
	 */
	string toText(const bytes data, const number BLOCK_SIZE);

	/**
	 * @brief write key to a binary file
	 *
	 * @param key the key to write
	 * @param filename the name of the file to write to
	 */
	void storeKey(const bytes key, const string filename);

	/**
	 * @brief read key from a binary file
	 *
	 * @param filename the name of the file to read from
	 * @return bytes the key read (KEYSIZE length)
	 */
	bytes loadKey(const string filename);

	/**
	 * @brief compute a hash of the given input using OpenSSL algorithms.
	 * SHA-2 family is used, disgest size in bits is HASHSIZE macro (currently 256).
	 *
	 * @param input the message to disgest
	 * @param output the digest of the message
	 */
	void hash(const bytes &input, bytes &output);

	/**
	 * @brief compute a hash of the message in a form of a number from zero to max.
	 * Simply computes a SHA2 hash, takes first sizeof(number) bytes and returns this mod max.
	 *
	 * @param input the message to disgest
	 * @param max the exclusive max number of the output
	 * @return number the hash of the message as a number [0, max)
	 */
	number hashToNumber(const bytes &input, number max);
}
