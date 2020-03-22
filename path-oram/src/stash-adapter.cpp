#include "stash-adapter.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsStashAdapter::~AbsStashAdapter(){};

	InMemoryStashAdapter::~InMemoryStashAdapter()
	{
		// TODO
	}

	InMemoryStashAdapter::InMemoryStashAdapter(unsigned int capacity) :
		capacity(capacity)
	{
		auto cmp = [](pair<ulong, bytes> a, pair<ulong, bytes> b) { return a.first < b.first; };
		// this->stash = set<pair<ulong, bytes>, decltype(cmp)>();
		auto p = set<pair<ulong, bytes>, decltype(cmp)>();
	}

	ulong InMemoryPositionMapAdapter::get(ulong block)
	{
		this->checkCapacity(block);

		return this->map[block];
	}

	void InMemoryPositionMapAdapter::set(ulong block, ulong leaf)
	{
		this->checkCapacity(block);

		this->map[block] = leaf;
	}

	void InMemoryPositionMapAdapter::checkCapacity(unsigned int block)
	{
		if (block >= this->capacity)
		{
			throw boost::format("block %1% out of bound (capacity %2%)") % block % this->capacity;
		}
	}
}
