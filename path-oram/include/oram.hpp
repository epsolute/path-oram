#pragma once

#include "definitions.h"
#include "position-map-adapter.hpp"
#include "stash-adapter.hpp"
#include "storage-adapter.hpp"

#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace PathORAM
{
	using namespace std;

	class AbsPositionMapAdapter;

	/**
	 * @brief PathORAM class
	 *
	 * Needs to be instantiated with adapters (storage, position map and stash).
	 * Don't forget to garbage collect them separately.
	 *
	 */
	class ORAM
	{
		private:
		const shared_ptr<AbsStorageAdapter> storage;
		const shared_ptr<AbsPositionMapAdapter> map;
		const shared_ptr<AbsStashAdapter> stash;

		const number dataSize; // size of the "usable" portion of the block in bytes
		const number Z;		   // number of blocks per bucket

		const number height;  // number of tree levels
		const number buckets; // total number of buckets
		const number blocks;  // total number of blocks

		const number batchSize; // a max number of requests to process at a time (default 1)

		// a layer between (expensive) storage and the protocol;
		// holds items (buckets of blocks) in memory and unencrypted;
		unordered_map<number, bucket> cache;

		/**
		 * @brief performs a single access, read or write
		 *
		 * @param read true of read access, false if write access
		 * @param block the block ID requested
		 * @param data if write, the data to be put in block (discarded if read)
		 * @param response if read, the content of requested block (empty if write)
		 */
		void access(const bool read, const number block, const bytes &data, bytes &response);

		/**
		 * @brief puts a path into the stash
		 *
		 * @param leaf the leaf that uniquely defines the path from root.
		 * Leaves are numbered from 0 to N.
		 * @param response path locations of the blocks in the path (will be populated)
		 * @param putInStash if set, the path will be read from storage and put in stash.
		 * Otherwise, will only populate the locations of blocks in the path.
		 */
		void readPath(const number leaf, unordered_set<number> &path, const bool putInStash);

		/**
		 * @brief write a path using the blocks from stash
		 *
		 * @param leaf the leaf that uniquely defines the path from root.
		 * Leaves are numbered from 0 to N.
		 */
		void writePath(const number leaf);

		/**
		 * @brief checks if the paths "merge" on the level
		 *
		 * @param pathLeaf leaf that defines the first path
		 * @param blockPosition leaf that defines the second path
		 * @param level level in question
		 * @return true if the paths share the same node on the given level
		 * @return false otherwise
		 */
		bool canInclude(const number pathLeaf, const number blockPosition, const number level) const;

		/**
		 * @brief computes the location in the storage for a bucket (not block) in a given path on a given level
		 *
		 * @param level level in question
		 * @param leaf leaf that defines the path in question
		 * @return number the location of the requested bucket (not block) in the storage
		 */
		number bucketForLevelLeaf(const number level, const number leaf) const;

		/**
		 * @brief make GET requests to the storage through cache.
		 * That is, upon the cache miss the item will be downloaded and stored in cache.
		 *
		 * @param locations the addresses of the blocks to read
		 * @param response the read blocks split into ORAM id and payload
		 * @param dryRun if set, will not populate response (will only download and put in interanal cache)
		 */
		void getCache(const unordered_set<number> &locations, vector<block> &response, const bool dryRun);

		/**
		 * @brief make SET requests to the storage through cache.
		 * This will NOT update the storage, only the cache (see syncCache).
		 *
		 * @param requests the set requests in a form of {address, {bucket of {ORAM ID, payload}}}
		 */
		void setCache(const vector<pair<number, bucket>> &requests);

		/**
		 * @brief upload all cache content to the storage and empty the cache
		 */
		void syncCache();

		friend class ORAMTest_BucketFromLevelLeaf_Test;
		friend class ORAMTest_CanInclude_Test;
		friend class ORAMTest_ReadPath_Test;
		friend class ORAMTest_ConsistencyCheck_Test;
		friend class ORAMTest_MultipleCheckCache_Test;
		friend class ORAMTest_MultipleGetNoDuplicates_Test;
		friend class ORAMBigTest;

		public:
		/**
		 * @brief Construct a new ORAM object given adapters
		 *
		 * \note
		 * You are responsible for the lifetime of the adapters (i.e. delete them!)
		 *
		 * @param logCapacity height of the tree or logarithm base 2 of capacity (i.e. capacity is 2 to the power of this value)
		 * @param blockSize the size (user's portion) of ORAM block in bytes (must be at least 2 AES block sizes - at least 32 bytes)
		 * @param Z number of blocks in a bucket (typically, 3 to 7)
		 * @param storage pointer to storage adapter to use
		 * @param map pointer to position map adapter to use
		 * @param stash pointer to stash adapter to use
		 * @param initialize whether to initialize map and storage (should be false if map and storage are read from files)
		 * @param batchSize controls the max number of requests in multiple(...)
		 */
		ORAM(
			const number logCapacity,
			const number blockSize,
			const number Z,
			const shared_ptr<AbsStorageAdapter> storage,
			const shared_ptr<AbsPositionMapAdapter> map,
			const shared_ptr<AbsStashAdapter> stash,
			const bool initialize  = true,
			const number batchSize = 1);

		/**
		 * @brief Construct a new ORAM object with adapters created automatically
		 *
		 * This is the same as the extended constructor, except the adapters are created automatically.
		 * They are destroyed when the ORAM instance is destroyed.
		 * The adapters are created with the following capcities:
		 * CAPACITY = 2^logCapacity
		 * 	in-memory storage: CAPACITY * Z + Z
		 * 	in-memory position map: CAPACITY * Z + Z
		 * 	in-memory stash: 3 * Z * logCapacity
		 *
		 * @param logCapacity as in the extended constructor
		 * @param blockSize as in the extended constructor
		 * @param Z as in the extended constructor
		 */
		ORAM(const number logCapacity, const number blockSize, const number Z);

		/**
		 * @brief Retrives a block from ORAM
		 *
		 * @param block block ID to request
		 * @param response the (decrypted) data from the block
		 */
		void get(const number block, bytes &response);

		/**
		 * @brief Puts a block to ORAM
		 *
		 * @param block block ID to request
		 * @param data the (plaintext) data to put in the block
		 */
		void put(const number block, const bytes &data);

		/**
		 * @brief processes multiple requests at a time
		 *
		 * \note
		 * If a sequence contains duplicates, those duplicate requests will miss the cache.
		 * The answer will still be correct, and operation secure, just not optimal.
		 *
		 * @param requests the sequence of requests in a form of {ID, payload}
		 * If payload is empty (zero size), the requests is treated as GET, otherwise PUT.
		 * @param response the answer to the requests.
		 * Matches the order of requests.
		 * For a GET request the answer is a payload for ID.
		 * For a PUT request the supplied payload is returned.
		 *
		 * \note
		 * The number fo request must not exceed the batchSize parameter used to construct the ORAM.
		 */
		void multiple(const vector<block> &requests, vector<bytes> &response);

		/**
		 * @brief bulk loads the data bypassing usual ORAM protocol
		 *
		 * Loads the data straight to the storage preserving ORAM invariant.
		 * Shuffles the data before inserting (to hide the original order).
		 * For each record, chooses random leaf and greedily fills the path from the leaf to the root.
		 * Tries random leaves until the record can be inserted in the path.
		 * Throws exception is ORAM is full (cannot insert in any path).
		 *
		 * \note
		 * Should only be used for off-line storage generation.
		 * For example, a client generates sotrage file on its machine and upload this file to the untrusted server.
		 * After that the usual ORAM accesses ocur.
		 *
		 * @param data the data to bulk load
		 */
		void load(const vector<block> &data);
	};
}
