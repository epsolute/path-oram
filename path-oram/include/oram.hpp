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

		ulong dataSize;
		ulong Z;

		ulong height;
		ulong buckets;
		ulong blocks;

		bool ownDependencies = false;

		bytes access(bool read, ulong block, bytes data);
		void readPath(ulong leaf);
		void writePath(ulong leaf);

		bool canInclude(ulong pathLeaf, ulong blockPosition, ulong level);
		ulong bucketForLevelLeaf(ulong level, ulong leaf);

		friend class ORAMTest_BucketFromLevelLeaf_Test;
		friend class ORAMTest_CanInclude_Test;
		friend class ORAMTest_ReadPath_Test;
		friend class ORAMTest_ConsistencyCheck_Test;
		friend class ORAMBigTest;

		public:
		ORAM(ulong logCapacity, ulong blockSize, ulong Z, AbsStorageAdapter *storage, AbsPositionMapAdapter *map, AbsStashAdapter *stash);
		ORAM(ulong logCapacity, ulong blockSize, ulong Z);
		~ORAM();

		bytes get(ulong block);
		void put(ulong block, bytes data);
	};
}
