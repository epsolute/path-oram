#pragma once

#include "definitions.h"

#include <aerospike/aerospike.h>
#include <fstream>
#include <sw/redis++/redis++.h>

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

		friend class StorageAdapterTest_GetSetInternal_Test;

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
		 * @brief retrives the data in batch
		 *
		 * \note
		 * Particular storage provider may or may not support batching.
		 * If it does not, batch will be executed sequentially.
		 *
		 * @param locations the locations from which to read
		 * @return vector<pair<number, bytes>> retrived data broken up into IDs and decrypted payloads
		 */
		virtual vector<pair<number, bytes>> get(vector<number> locations);

		/**
		 * @brief writes the data in batch
		 *
		 * \note
		 * Particular storage provider may or may not support batching.
		 * If it does not, batch will be executed sequentially.
		 *
		 * @param requests locations and data requests (IDs and payloads) to write
		 */
		virtual void set(vector<pair<number, pair<number, bytes>>> requests);

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

		/**
		 * @brief batch version of setInternal
		 *
		 * @param requests sequency of blocks to write (location, raw bytes)
		 */
		virtual void setInternal(vector<pair<number, bytes>> requests);

		/**
		 * @brief batch version of getInternal
		 *
		 * @param locations sequence (ordered) of locations to read from
		 * @return vector<bytes> blocks of bytes in the order defined by locations
		 */
		virtual vector<bytes> getInternal(vector<number> locations);
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

		friend class MockStorage;
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

	/**
	 * @brief Redis implementation of the storage adapter.
	 *
	 * Uses a Redis cluster as the underlying storage.
	 */
	class RedisStorageAdapter : public AbsStorageAdapter
	{
		private:
		unique_ptr<sw::redis::Redis> redis;

		public:
		/**
		 * @brief Construct a new Redis Storage Adapter object
		 *
		 * It is possible to persist the data.
		 * If the file exists, instantiate with override = false, and the key equal to the one used before.
		 *
		 * @param capacity the max number of blocks
		 * @param userBlockSize the size of the user's portion of the block in bytes
		 * @param key the AES key to use (may be empty to generate new random one)
		 * @param host the URL to the Redis cluster (will throw exception if ping on the URL fails)
		 * @param override if true, the cluster will be flushed and filled with random blocks first
		 */
		RedisStorageAdapter(number capacity, number userBlockSize, bytes key, string host, bool override);
		~RedisStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;

		void setInternal(vector<pair<number, bytes>> requests) final;
		vector<bytes> getInternal(vector<number> locations) final;
	};

	/**
	 * @brief Aerospike implementation of the storage adapter.
	 *
	 * Uses an Aerospike cluster as the underlying storage.
	 */
	class AerospikeStorageAdapter : public AbsStorageAdapter
	{
		private:
		aerospike as;
		string asset;

		/**
		 * @brief remove all records from the own set
		 *
		 */
		void deleteAll();

		friend class ORAMBigTest;

		public:
		/**
		 * @brief Construct a new Aerospike Storage Adapter object
		 *
		 * It is possible to persist the data.
		 * If the file exists, instantiate with override = false, and the key equal to the one used before.
		 *
		 * @param capacity the max number of blocks
		 * @param userBlockSize the size of the user's portion of the block in bytes
		 * @param key the AES key to use (may be empty to generate new random one)
		 * @param host the URL to the Aerospike cluster (will throw exception if ping on the URL fails)
		 * @param override if true, the cluster will be flushed and filled with random blocks first
		 * @param set specifies the set to use for all operations
		 */
		AerospikeStorageAdapter(number capacity, number userBlockSize, bytes key, string host, bool override, string set = "default");
		~AerospikeStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;

		vector<bytes> getInternal(vector<number> locations) final;
	};
}
