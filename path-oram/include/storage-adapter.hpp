#pragma once

#include "definitions.h"

#include <aerospike/aerospike.h>
#include <boost/signals2/signal.hpp>
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
		using OnStorageRequest = boost::signals2::signal<void(bool read, number batch, number size, number overhead)>;

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

		/**
		 * @brief Proxy for setInternal(number location, bytes raw) that emits OnStorageRequest
		 */
		void setAndRecord(number location, bytes raw);

		/**
		 * @brief Proxy for getInternal(number location) that emits OnStorageRequest
		 */
		bytes getAndRecord(number location);

		/**
		 * @brief Proxy for setInternal(vector<pair<number, bytes>> requests) that emits OnStorageRequest
		 */
		void setAndRecord(vector<pair<number, bytes>> requests);

		/**
		 * @brief Proxy for getInternal(vector<number> locations) that emits OnStorageRequest
		 */
		vector<bytes> getAndRecord(vector<number> locations);

		bytes key; // AES key for encryption operations
		number Z;  // number of blocks in a bucket

		// Event handler
		OnStorageRequest onStorageRequest;

		friend class StorageAdapterTest_GetSetInternal_Test;
		friend class MockStorage;

		public:
		/**
		 * @brief retrieves the data from the location
		 *
		 * @param location location in question
		 * @return bucket retrived data broken up into Z blocks {ID, decrypted payload}
		 */
		bucket get(number location);

		/**
		 * @brief writes the data to the location
		 *
		 * It will encrypt the data before putting it to storage.
		 *
		 * @param location location in question
		 * @param data composition of Z blocks {ID, plaintext payload}
		 */
		void set(number location, bucket data);

		/**
		 * @brief Subscribes to OnStorageRequest notifications.
		 *
		 * @param handler the handler to execute with event
		 * @return boost::signals2::connection the connection object (to be used for unsubscribing)
		 */
		boost::signals2::connection subscribe(const OnStorageRequest::slot_type &handler);

		/**
		 * @brief retrives the data in batch
		 *
		 * \note
		 * Particular storage provider may or may not support batching.
		 * If it does not, batch will be executed sequentially.
		 *
		 * @param locations the locations from which to read
		 * @return vector<block> retrived data broken up into IDs and decrypted payloads
		 */
		vector<block> get(vector<number> locations);

		/**
		 * @brief writes the data in batch
		 *
		 * \note
		 * Particular storage provider may or may not support batching.
		 * If it does not, batch will be executed sequentially.
		 *
		 * @param requests locations and data requests (IDs and payloads) to write
		 */
		void set(vector<pair<number, bucket>> requests);

		/**
		 * @brief sets all available locations (given by CAPACITY) to zeroed bytes.
		 * On the storage these zeroes will appear randomized encrypted.
		 */
		void fillWithZeroes();

		/**
		 * @brief whether this adapter supports batch read operations.
		 */
		virtual const bool supportsBatchGet() = 0;

		/**
		 * @brief whether this adapter supports batch write operations.
		 */
		virtual const bool supportsBatchSet() = 0;

		/**
		 * @brief Construct a new Abs Storage Adapter object
		 *
		 * @param capacity the maximum number of buckets (not blocks) to store
		 * @param userBlockSize the size of the user's potion (payload) of the block in bytes.
		 * Has to be at least two AES blocks (32 bytes) and be a multiple of AES block (16 bytes).
		 *
		 * @param key AES key to use for encryption.
		 * Has to be KEYSIZE bytes (32), otherwise will be generated randomly.
		 *
		 * @param Z the number of blocks in a bucket.
		 * GET and SET will operate using Z.
		 */
		AbsStorageAdapter(number capacity, number userBlockSize, bytes key, number Z);
		virtual ~AbsStorageAdapter() = 0;

		protected:
		number capacity;	  // number of buckets
		number blockSize;	  // whole bucket size (Z times (user portion + ID) + IV)
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
		InMemoryStorageAdapter(number capacity, number userBlockSize, bytes key, number Z);
		~InMemoryStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;

		const bool supportsBatchGet() final { return false; };
		const bool supportsBatchSet() final { return false; };

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
		 * @param Z the number of blocks in a bucket.
		 * GET and SET will operate using Z.
		 */
		FileSystemStorageAdapter(number capacity, number userBlockSize, bytes key, string filename, bool override, number Z);
		~FileSystemStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;

		const bool supportsBatchGet() final { return false; };
		const bool supportsBatchSet() final { return false; };
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
		 * @param Z the number of blocks in a bucket.
		 * GET and SET will operate using Z.
		 */
		RedisStorageAdapter(number capacity, number userBlockSize, bytes key, string host, bool override, number Z);
		~RedisStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;

		void setInternal(vector<pair<number, bytes>> requests) final;
		vector<bytes> getInternal(vector<number> locations) final;

		const bool supportsBatchGet() final { return true; };
		const bool supportsBatchSet() final { return true; };
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
		 * @param Z the number of blocks in a bucket.
		 * GET and SET will operate using Z.
		 */
		AerospikeStorageAdapter(number capacity, number userBlockSize, bytes key, string host, bool override, number Z, string set = "default");
		~AerospikeStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;

		vector<bytes> getInternal(vector<number> locations) final;

		const bool supportsBatchGet() final { return true; };
		const bool supportsBatchSet() final { return false; };
	};
}
