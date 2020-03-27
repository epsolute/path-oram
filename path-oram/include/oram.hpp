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

	class ORAM
	{
		private:
		AbsStorageAdapter *storage;
		AbsPositionMapAdapter *map;
		AbsStashAdapter *stash;

		number dataSize;
		number Z;

		number height;
		number buckets;
		number blocks;

		bool ownDependencies = false;

		bytes access(bool read, number block, bytes data);
		void readPath(number leaf);
		void writePath(number leaf);

		bool canInclude(number pathLeaf, number blockPosition, number level);
		number bucketForLevelLeaf(number level, number leaf);

		friend class ORAMTest_BucketFromLevelLeaf_Test;
		friend class ORAMTest_CanInclude_Test;
		friend class ORAMTest_ReadPath_Test;
		friend class ORAMTest_ConsistencyCheck_Test;
		friend class ORAMBigTest;

		public:
		ORAM(number logCapacity, number blockSize, number Z, AbsStorageAdapter *storage, AbsPositionMapAdapter *map, AbsStashAdapter *stash);
		ORAM(number logCapacity, number blockSize, number Z);
		~ORAM();

		bytes get(number block);
		void put(number block, bytes data);
	};
}
