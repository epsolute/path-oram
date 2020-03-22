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
		this->height  = logCapacity;							// we are given a height
		this->blocks  = (ulong)1 << this->height;				// number of blocks is 2^height
		this->buckets = (this->blocks + this->Z - 1) / this->Z; // number of buckets is block / Z (Z blocks per bucket)
		this->stash.clear();

		// cout << logCapacity << endl;
		// cout << blockSize << endl;
		// cout << Z << endl;
		// cout << height << endl;
		// cout << buckets << endl;
		// cout << blocks << endl;

		// fill all blocks with random bits
		for (ulong i = 0uLL; i < this->buckets; i++)
		{
			for (ulong j = 0uLL; j < this->Z; j++)
			{
				auto block = getRandomBlock(this->blockSize);
				this->storage->set(i * this->Z + j, block);
			}
		}

		// generate random position map
		for (ulong i = 0; i < this->blocks; ++i)
		{
			this->map->set(i, getRandomULong(this->blocks));
		}
	}

	ORAM::~ORAM() {}

	bytes ORAM::get(ulong block)
	{
		return this->access(true, block, bytes());
	}

	void ORAM::set(ulong block, bytes data)
	{
		this->access(false, block, data);
	}

	bytes ORAM::access(bool read, ulong block, bytes data)
	{
		// step 1 from paper: remap block
		auto previousPosition = this->map->get(block);
		this->map->set(block, getRandomULong(this->blocks));

		// step 2 from paper: read path
		this->readPath(previousPosition); // stash updated

		// step 3 from paper: update block
		if (!read) // if "write"
		{
			this->stash[block] = data;
		}
		auto returned = this->stash[block];

		// step 4 from paper: write path
		writePath(previousPosition); // stash updated

		return returned;
	}

	void ORAM::readPath(ulong leaf)
	{
		for (ulong position = leaf + (1 << (height - 1)); position > 0; position >>= 1)
		{
			for (ulong i = 0; i < this->Z; i++)
			{
				auto block		   = (position - 1) * this->Z + i;
				this->stash[block] = this->storage->get(block);
			}
		}
	}

	void ORAM::writePath(ulong leaf)
	{
		for (ulong position = leaf + (1 << (height - 1)); position > 0; position >>= 1)
		{
			for (ulong i = 0; i < this->Z; i++)
			{
				auto block		   = (position - 1) * this->Z + i;
				// TODO
				this->storage->set(block, this->stash[block]);
			}
		}
	}

	bool ORAM::canInclude(ulong pathLeaf, ulong blockPosition, ulong level)
	{
		return (pathLeaf >> level) == (blockPosition >> level);
	}
}
