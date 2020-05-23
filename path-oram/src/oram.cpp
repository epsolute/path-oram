#include "oram.hpp"

#include "utility.hpp"

#include <boost/format.hpp>
#include <unordered_set>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	ORAM::ORAM(number logCapacity, number blockSize, number Z, shared_ptr<AbsStorageAdapter> storage, shared_ptr<AbsPositionMapAdapter> map, shared_ptr<AbsStashAdapter> stash, bool initialize, number batchSize) :
		storage(storage),
		map(map),
		stash(stash),
		dataSize(blockSize),
		Z(Z),
		batchSize(batchSize)
	{
		this->height  = logCapacity;		 // we are given a height
		this->buckets = (number)1 << height; // number of buckets is 2^height
		this->blocks  = buckets * Z;		 // number of blocks is buckets / Z (Z blocks per bucket)

		if (initialize)
		{
			// fill all blocks with random bits, marks them as "empty"
			storage->fillWithZeroes();

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
			 make_shared<InMemoryStorageAdapter>((1 << logCapacity), blockSize * Z, bytes(), Z),
			 make_shared<InMemoryPositionMapAdapter>(((1 << logCapacity) * Z) + Z),
			 make_shared<InMemoryStashAdapter>(3 * logCapacity * Z))
	{
	}

	bytes ORAM::get(number block)
	{
		auto result = access(true, block, bytes());
		syncCache();
		return result;
	}

	void ORAM::put(number block, bytes data)
	{
		access(false, block, data);
		syncCache();
	}

	vector<bytes> ORAM::multiple(vector<block> requests)
	{
		if (requests.size() > batchSize)
		{
			throw Exception(boost::format("Too many requests (%1%) for batch size %2%") % requests.size() % batchSize);
		}

		// populate cache
		unordered_set<number> locationsSet;
		for (auto request : requests)
		{
			auto path = readPath(map->get(request.first), false);
			locationsSet.insert(path.begin(), path.end());
		}

		vector<number> locations(locationsSet.begin(), locationsSet.end());
		getCache(locations);

		// run ORAM protocol (will use cache)
		vector<bytes> results;
		transform(
			requests.begin(),
			requests.end(),
			back_inserter(results),
			[this](block request) { return access(request.second.size() == 0, request.first, request.second); });

		// upload resulting new data
		syncCache();

		return results;
	}

	void ORAM::load(vector<block> data)
	{
		unordered_map<number, bucket> localStorage;
		auto emptyBucket = vector<block>();
		for (auto i = 0uLL; i < Z; i++)
		{
			emptyBucket.push_back({ULONG_MAX, bytes()});
		}

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
					auto bucketId = bucketForLevelLeaf(level, leaf);
					auto bucket	  = localStorage.count(bucketId) == 0 ? emptyBucket : localStorage[bucketId];

					for (number i = 0; i < Z; i++)
					{
						// if empty
						if (bucket[i].first == ULONG_MAX)
						{
							bucket[i]			   = record;
							localStorage[bucketId] = bucket;
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

		vector<pair<number, bucket>> requests;
		copy(localStorage.begin(), localStorage.end(), back_inserter(requests));
		storage->set(requests);
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

	vector<number> ORAM::readPath(number leaf, bool putInStash)
	{
		vector<number> requests;

		// for levels from root to leaf
		for (number level = 0; level < height; level++)
		{
			auto bucket = bucketForLevelLeaf(level, leaf);
			requests.push_back(bucket);
		}

		// we may only want to populate cache
		if (putInStash)
		{
			auto blocks = getCache(requests);

			for (auto [id, data] : blocks)
			{
				// skip "empty" buckets
				if (id != ULONG_MAX)
				{
					stash->add(id, data);
				}
			}
		}

		return requests;
	}

	void ORAM::writePath(number leaf)
	{
		vector<block> currentStash;
		stash->getAll(currentStash);
		vector<int> toDelete;				   // rember the records that will need to be deleted from stash
		vector<pair<number, bucket>> requests; // storage SET requests (batching)

		// following the path from leaf to root (greedy)
		for (int level = height - 1; level >= 0; level--)
		{
			vector<block> toInsert;		  // block to be insterted in the bucket (up to Z)
			vector<number> toDeleteLocal; // same blocks needs to be deleted from stash (these hold indices of elements in currentStash)
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

			auto bucketId = bucketForLevelLeaf(level, leaf);
			bucket bucket;
			bucket.resize(Z);

			// write the bucket
			for (number i = 0; i < Z; i++)
			{
				// auto block = bucket * Z + i;
				if (toInsert.size() != 0)
				{
					auto data = toInsert.back();
					toInsert.pop_back();
					bucket[i] = data;
				}
				else
				{
					// if nothing to insert, insert dummy (for security)
					bucket[i] = {ULONG_MAX, getRandomBlock(dataSize)};
				}
			}

			requests.push_back({bucketId, bucket});
		}

		setCache(requests);

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

	vector<block> ORAM::getCache(vector<number> locations)
	{
		vector<block> result;

		// get those locations not present in the cache
		vector<number> toGet;
		for (auto &&location : locations)
		{
			auto bucketIt = cache.find(location);
			if (bucketIt == cache.end())
			{
				toGet.push_back(location);
			}
			else
			{
				result.insert(result.begin(), (*bucketIt).second.begin(), (*bucketIt).second.end());
			}
		}

		if (toGet.size() > 0)
		{
			// download those blocks
			vector<block> downloaded;
			storage->get(toGet, downloaded);

			// add them to the cache and the result
			bucket bucket;
			for (auto i = 0uLL; i < downloaded.size(); i++)
			{
				result.push_back(downloaded[i]);
				bucket.push_back(downloaded[i]);
				if (i % Z == Z - 1)
				{
					cache[toGet[i / Z]] = bucket;
					bucket.clear();
				}
			}
		}

		return result;
	}

	void ORAM::setCache(vector<pair<number, bucket>> requests)
	{
		for (auto request : requests)
		{
			cache[request.first] = request.second;
		}
	}

	void ORAM::syncCache()
	{
		// convert map to vector of pairs
		vector<pair<number, bucket>> requests;

		copy(cache.begin(), cache.end(), back_inserter(requests));

		storage->set(requests);

		cache.clear();
	}
}
