#include "oram.hpp"

#include "utility.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	ORAM::ORAM(number logCapacity, number blockSize, number Z, AbsStorageAdapter* storage, AbsPositionMapAdapter* map, AbsStashAdapter* stash) :
		storage(storage),
		map(map),
		stash(stash),
		dataSize(blockSize),
		Z(Z)
	{
		this->height  = logCapacity;		 // we are given a height
		this->buckets = (number)1 << height; // number of buckets is 2^height
		this->blocks  = buckets * Z;		 // number of blocks is buckets / Z (Z blocks per bucket)

		// fill all blocks with random bits
		for (number i = 0uLL; i < buckets; i++)
		{
			for (number j = 0uLL; j < Z; j++)
			{
				auto block = getRandomBlock(dataSize);
				storage->set(i * Z + j, {ULONG_MAX, block});
			}
		}

		// generate random position map
		for (number i = 0; i < blocks; ++i)
		{
			map->set(i, getRandomULong(1 << (height - 1)));
		}
	}

	ORAM::ORAM(number logCapacity, number blockSize, number Z) :
		ORAM(logCapacity,
			 blockSize,
			 Z,
			 new InMemoryStorageAdapter(((1 << logCapacity) * Z) + Z, blockSize, bytes()),
			 new InMemoryPositionMapAdapter(((1 << logCapacity) * Z) + Z),
			 new InMemoryStashAdapter(3 * logCapacity * Z))
	{
		ownDependencies = true;
	}

	ORAM::~ORAM()
	{
		if (ownDependencies)
		{
			delete storage;
			delete map;
			delete stash;
		}
	}

	bytes ORAM::get(number block)
	{
		return access(true, block, bytes());
	}

	void ORAM::put(number block, bytes data)
	{
		access(false, block, data);
	}

	bytes ORAM::access(bool read, number block, bytes data)
	{
		// step 1 from paper: remap block
		auto previousPosition = map->get(block);
		map->set(block, getRandomULong(1 << (height - 1)));

		// step 2 from paper: read path
		readPath(previousPosition); // stash updated

		// step 3 from paper: update block
		if (!read) // if "write"
		{
			stash->update(block, data);
		}
		auto returned = stash->get(block);

		// step 4 from paper: write path
		writePath(previousPosition); // stash updated

		return returned;
	}

	void ORAM::readPath(number leaf)
	{
		for (number level = 0; level < height; level++)
		{
			auto bucket = bucketForLevelLeaf(level, leaf);
			for (number i = 0; i < Z; i++)
			{
				auto block		= bucket * Z + i;
				auto [id, data] = storage->get(block);
				if (id != ULONG_MAX)
				{
					stash->add(id, data);
				}
			}
		}
	}

	void ORAM::writePath(number leaf)
	{
		auto currentStash = stash->getAll();
		vector<int> toDelete;

		for (int level = height - 1; level >= 0; level--)
		{
			vector<pair<number, bytes>> toInsert;
			vector<int> toDeleteLocal;
			for (auto entry : currentStash)
			{
				auto entryLeaf = map->get(entry.first);
				if (canInclude(entryLeaf, leaf, level))
				{
					toInsert.push_back(entry);
					toDelete.push_back(entry.first);
					toDeleteLocal.push_back(entry.first);
					if (toInsert.size() == Z)
					{
						break;
					}
				}
			}
			for (auto removed : toDeleteLocal)
			{
				currentStash.erase(removed);
			}

			auto bucket = bucketForLevelLeaf(level, leaf);

			for (number i = 0; i < Z; i++)
			{
				auto block = bucket * Z + i;
				if (toInsert.size() != 0)
				{
					auto data = toInsert.back();
					toInsert.pop_back();
					storage->set(block, data);
				}
				else
				{
					storage->set(block, {ULONG_MAX, getRandomBlock(dataSize)});
				}
			}
		}

		for (auto removed : toDelete)
		{
			stash->remove(removed);
		}
	}

	number ORAM::bucketForLevelLeaf(number level, number leaf)
	{
		return (leaf + (1 << (height - 1))) >> (height - 1 - level);
	}

	bool ORAM::canInclude(number pathLeaf, number blockPosition, number level)
	{
		return bucketForLevelLeaf(level, pathLeaf) == bucketForLevelLeaf(level, blockPosition);
	}
}
