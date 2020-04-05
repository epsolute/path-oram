#pragma once

#include "definitions.h"
#include "position-map-adapter.hpp"
#include "stash-adapter.hpp"
#include "storage-adapter.hpp"

#include <iostream>
#include <unordered_map>

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
		AbsStorageAdapter *storage;
		AbsPositionMapAdapter *map;
		AbsStashAdapter *stash;

		number dataSize; // size of the "usable" portion of the block in bytes
		number Z;		 // number of blocks per bucket

		number height;	// number of tree levels
		number buckets; // total number of buckets
		number blocks;	// total number of blocks

		bool ownDependencies = false; // if true, will delete adapters in destructor

		/**
		 * @brief performs a single access, read or write
		 *
		 * @param read true of read access, false if write access
		 * @param block the block ID requested
		 * @param data if write, the data to be put in block (discarded if read)
		 * @return bytes if read, the content of requested block (empty if write)
		 */
		bytes access(bool read, number block, bytes data);

		/**
		 * @brief puts a path into the stash
		 *
		 * @param leaf the leaf that uniquely defines the path from root.
		 * Leaves are numbered from 0 to N.
		 */
		void readPath(number leaf);

		/**
		 * @brief write a path using the blocks from stash
		 *
		 * @param leaf the leaf that uniquely defines the path from root.
		 * Leaves are numbered from 0 to N.
		 */
		void writePath(number leaf);

		/**
		 * @brief checks if the paths "merge" on the level
		 *
		 * @param pathLeaf leaf that defines the first path
		 * @param blockPosition leaf that defines the second path
		 * @param level level in question
		 * @return true if the paths share the same node on the given level
		 * @return false otherwise
		 */
		bool canInclude(number pathLeaf, number blockPosition, number level);

		/**
		 * @brief computes the location in the storage for a bucket (not block) in a given path on a given level
		 *
		 * @param level level in question
		 * @param leaf leaf that defines the path in question
		 * @return number the location of the requested bucket (not block) in the storage
		 */
		number bucketForLevelLeaf(number level, number leaf);

		friend class ORAMTest_BucketFromLevelLeaf_Test;
		friend class ORAMTest_CanInclude_Test;
		friend class ORAMTest_ReadPath_Test;
		friend class ORAMTest_ConsistencyCheck_Test;
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
		 */
		ORAM(number logCapacity, number blockSize, number Z, AbsStorageAdapter *storage, AbsPositionMapAdapter *map, AbsStashAdapter *stash, bool initialize = true);

		/**
		 * @brief Construct a new ORAM object with adapters created automatically
		 *
		 * This is the same as the extended constructor, except the adapters are created automatically.
		 * They are destroyed when the ORAM instance is destroyed.
		 * The adapters are created with the following capcities:
		 * CAPACITY = 2^logCapacity * Z
		 * 	in-memory storage: CAPACITY + Z
		 * 	in-memory position map: CAPACITY + Z
		 * 	in-memory stash: 3 * Z * logCapacity
		 *
		 * @param logCapacity as in the extended constructor
		 * @param blockSize as in the extended constructor
		 * @param Z as in the extended constructor
		 */
		ORAM(number logCapacity, number blockSize, number Z);

		/**
		 * @brief Destroy the ORAM object
		 *
		 * Also destroys adapters if shorthand constructor was used.
		 * (Essentially, the one who owns is the one who destroys.)
		 */
		~ORAM();

		/**
		 * @brief Retrives a block from ORAM
		 *
		 * @param block block ID to request
		 * @return bytes the (decrypted) data from the block
		 */
		bytes get(number block);

		/**
		 * @brief Puts a block to ORAM
		 *
		 * @param block block ID to request
		 * @param data the (plaintext) data to put in the block
		 */
		void put(number block, bytes data);

		/**
		 * @brief bulk loads the data bypassing ORAM access
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
		void load(vector<pair<number, bytes>> data);
	};
}
