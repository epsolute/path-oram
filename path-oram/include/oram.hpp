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

		bytes access(bool read, ulong block, bytes data);

		public:
		ORAM(ulong logCapacity, ulong blockSize, ulong Z, AbsStorageAdapter *storage, AbsPositionMapAdapter *map);
		~ORAM();

		bytes get(ulong block);
		void set(ulong block, bytes data);
	};
}
