#pragma once

#include "definitions.h"

#include <fstream>

namespace PathORAM
{
	using namespace std;

	/**
	 * @brief An abstraction over storage adapter
	 *
	 * The format of the underlying block is the following.
	 * AES block size (16) bytes of IV, the rest is ciphertext.
	 * The ciphertext is AES block size (16) of ID (padded), the rest is user's payload.
	 */
	class AbsStorageAdapter
	{
		private:
		/**
		 * @brief throws exception if a requested location is outside of capcity
		 *
		 * @param location location in question
		 */
		void checkCapacity(number location);

		/**
		 * @brief check if a given data size is not above the block size
		 *
		 * @param dataSize size of the data to be inserted
		 */
		void checkBlockSize(number dataSize);

		bytes key; // AES key for encryption operations

		public:
		/**
		 * @brief retrieves the data from the location
		 *
		 * @param location location in question
		 * @return pair<number, bytes> retrived data broken up into ID and decrypted payload
		 */
		pair<number, bytes> get(number location);

		/**
		 * @brief writes the data to the location
		 *
		 * It will encrypt the data before putting it to storage.
		 *
		 * @param location location in question
		 * @param data composition of ID and plaintext payload
		 */
		void set(number location, pair<number, bytes> data);

		/**
		 * @brief Construct a new Abs Storage Adapter object
		 *
		 * @param capacity the maximum number of block to store
		 * @param userBlockSize the size of the user's potion (payload) of the block in bytes.
		 * Has to be at least two AES blocks (32 bytes) and be multiple of AES block (16 bytes).
		 *
		 * @param key AES key to use for encryption.
		 * Has to be KEYSIZE bytes (32), otherwise will be generated randomly.
		 */
		AbsStorageAdapter(number capacity, number userBlockSize, bytes key);
		virtual ~AbsStorageAdapter() = 0;

		protected:
		number capacity;	  // number of blocks
		number blockSize;	  // whole block size (user portion + ID + IV)
		number userBlockSize; // number of bytes in payload portion of block

		/**
		 * @brief actual routine that writes raw bytes to storage
		 *
		 * Does not care about IDs or encryption.
		 *
		 * @param location where to write bytes
		 * @param raw the bytes to write
		 */
		virtual void setInternal(number location, bytes raw) = 0;

		/**
		 * @brief actual routine that retrieves raw bytes to storage
		 *
		 * @param location location from where to read bytes
		 * @return bytes the retrieved bytes
		 */
		virtual bytes getInternal(number location) = 0;
	};

	/**
	 * @brief In-memory implementation of the storage adapter.
	 *
	 * Uses a RAM array as the underlying storage.
	 */
	class InMemoryStorageAdapter : public AbsStorageAdapter
	{
		private:
		uchar **blocks;

		public:
		InMemoryStorageAdapter(number capacity, number userBlockSize, bytes key);
		~InMemoryStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;
	};

	/**
	 * @brief File system implementation of the storage adapter.
	 *
	 * Uses a binary file as the underlying storage.
	 */
	class FileSystemStorageAdapter : public AbsStorageAdapter
	{
		private:
		fstream file;

		public:
		/**
		 * @brief Construct a new File System Storage Adapter object
		 *
		 * It is possible to persist the data.
		 * If the file exists, instantiate with override = false, and the key equal to the one used before.
		 *
		 * @param capacity the max number of blocks
		 * @param userBlockSize the size of the user's portion of the block in bytes
		 * @param key the AES key to use (may be empty to generate new random one)
		 * @param filename the file path to use
		 * @param override if true, the file will be opened, otherwise it will be recreated
		 */
		FileSystemStorageAdapter(number capacity, number userBlockSize, bytes key, string filename, bool override);
		~FileSystemStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;
	};
}
