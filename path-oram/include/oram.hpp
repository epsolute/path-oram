#include "definitions.h"
#include "position-map-adapter.hpp"
#include "storage-adapter.hpp"

#include <iostream>
#include <unordered_map>

namespace PathORAM
{
	using namespace std;

	class ORAM
	{
		private:
		AbsStorageAdapter *storage;
		AbsPositionMapAdapter *map;
		unordered_map<ulong, bytes> stash;

		ulong blockSize;
		ulong Z;

		ulong height;
		ulong buckets;
		ulong blocks;

		bytes access(bool read, ulong block, bytes data);
		void readPath(ulong leaf);
		void writePath(ulong leaf);
		bool canInclude(ulong pathLeaf, ulong blockPosition, ulong level);

		public:
		ORAM(ulong logCapacity, ulong blockSize, ulong Z, AbsStorageAdapter *storage, AbsPositionMapAdapter *map);
		~ORAM();

		bytes get(ulong block);
		void set(ulong block, bytes data);
	};
}
