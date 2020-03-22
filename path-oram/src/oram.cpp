#include "oram.hpp"

#include "utility.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	ORAM::ORAM(ulong logCapacity, ulong blockSize, ulong Z, AbsStorageAdapter *storage, AbsPositionMapAdapter *map) :
		storage(storage),
		map(map),
		blockSize(blockSize),
		Z(Z)
	{
		height		= logCapacity + 1;
		auto blocks = (ulong)1 << (height - 1);
		buckets		= (blocks + Z - 1) / Z;
		stash.clear();

		// cout << logCapacity << endl;
		// cout << blockSize << endl;
		// cout << Z << endl;
		// cout << height << endl;
		// cout << buckets << endl;
		// cout << blocks << endl;

		for (ulong i = 0uLL; i < buckets; i++)
		{
			for (ulong j = 0uLL; j < Z; j++)
			{
				auto block = getRandomBlock(blockSize);
				storage->set(i * Z + j, block);
			}
		}

		for (ulong i = 0; i < buckets; ++i)
		{
			map->set(i, getRandomULong(buckets));
		}
	}

	ORAM::~ORAM() {}

	bytes ORAM::get(ulong block)
	{
		return bytes();
	}

	void ORAM::set(ulong block, bytes data)
	{
	}
}
