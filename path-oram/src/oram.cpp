#include "oram.hpp"

#include "utility.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	ORAM::ORAM(number logCapacity, number blockSize, number Z, AbsStorageAdapter* storage, AbsPositionMapAdapter* map, AbsStashAdapter* stash, bool initialize) :
		storage(storage),
		map(map),
		stash(stash),
		dataSize(blockSize),
		Z(Z)
	{
		this->height  = logCapacity;		 // we are given a height
		this->buckets = (number)1 << height; // number of buckets is 2^height
		this->blocks  = buckets * Z;		 // number of blocks is buckets / Z (Z blocks per bucket)

		if (initialize)
		{
			// fill all blocks with random bits, marks them as "empty"
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
	}

	ORAM::ORAM(number logCapacity, number blockSize, number Z) :
		ORAM(logCapacity,
			 blockSize,
			 Z,
			 new InMemoryStorageAdapter(((1 << logCapacity) * Z) + Z, blockSize, bytes()),
			 new InMemoryPositionMapAdapter(((1 << logCapacity) * Z) + Z),
			 new InMemoryStashAdapter(3 * logCapacity * Z))
	{
		// we created the adapters, we will destroy them
		ownDependencies = true;
	}

	ORAM::~ORAM()
	{
		// we created the adapters, we will destroy them
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

	void ORAM::load(vector<pair<number, bytes>> data)
	{
		// shuffle (such bulk load may leak in part the original order)
		uint n = data.size();
		if (n >= 2)
		{
			// Fisher-Yates shuffle
			for (uint i = 0; i < n - 1; i++)
			{
				uint j = i + getRandomUInt(n - i);
				swap(data[i], data[j]);
			}
		}

		for (auto record : data)
		{
			auto safeGuard = 0;
			while (true)
			{
				auto leaf = getRandomULong(1 << (height - 1));

				for (int level = height - 1; level >= 0; level--)
				{
					auto bucket = bucketForLevelLeaf(level, leaf);
					for (number i = 0; i < Z; i++)
					{
						auto block = bucket * Z + i;
						auto id	   = storage->get(block).first;
						if (id == ULONG_MAX)
						{
							storage->set(block, record);
							map->set(record.first, leaf);
							goto found;
						}
					}
				}
				if (safeGuard++ == (1 << (height - 1)))
				{
					throw Exception("no space left in ORAM for bulk load");
				}
			}
		found:
			continue;
		}
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
		// for levels from root to leaf
		for (number level = 0; level < height; level++)
		{
			// get the bucket and iterate over blocks in the bucket
			auto bucket = bucketForLevelLeaf(level, leaf);
			for (number i = 0; i < Z; i++)
			{
				auto block		= bucket * Z + i;
				auto [id, data] = storage->get(block);
				// skip "empty" buckets
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
		vector<int> toDelete; // rember the records that will need to be deleted from stash

		// following the path from leaf to root (greedy)
		for (int level = height - 1; level >= 0; level--)
		{
			vector<pair<number, bytes>> toInsert; // block to be insterted in the bucket (up to Z)
			vector<number> toDeleteLocal;		  // same blocks needs to be deleted from stash (these hold indices of elements in currentStash)
			for (number i = 0; i < currentStash.size(); i++)
			{
				auto entry	   = currentStash[i];
				auto entryLeaf = map->get(entry.first);
				// see if this block from stash fits in this bucket
				if (canInclude(entryLeaf, leaf, level))
				{
					toInsert.push_back(entry);
					toDelete.push_back(entry.first);

					toDeleteLocal.push_back(i);

					// look up to Z
					if (toInsert.size() == Z)
					{
						break;
					}
				}
			}
			// delete inserted blocks from local stash
			// we remove elements by location, so after operation vector shrinks (nasty bug...)
			sort(toDeleteLocal.begin(), toDeleteLocal.end(), greater<number>());
			for (auto removed : toDeleteLocal)
			{
				currentStash.erase(currentStash.begin() + removed);
			}

			auto bucket = bucketForLevelLeaf(level, leaf);

			// write the bucket
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
					// if nothing to insert, insert dummy (for security)
					storage->set(block, {ULONG_MAX, getRandomBlock(dataSize)});
				}
			}
		}

		// update the stash adapter, remove newly inserted blocks
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
		// on this level, do these paths share the same bucket
		return bucketForLevelLeaf(level, pathLeaf) == bucketForLevelLeaf(level, blockPosition);
	}
}
