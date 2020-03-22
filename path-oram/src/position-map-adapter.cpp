#include "position-map-adapter.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsPositionMapAdapter::~AbsPositionMapAdapter(){};

	InMemoryPositionMapAdapter::~InMemoryPositionMapAdapter()
	{
		delete[] this->map;
	}

	InMemoryPositionMapAdapter::InMemoryPositionMapAdapter(ulong capacity) :
		capacity(capacity)
	{
		this->map = new ulong[capacity];
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

	void InMemoryPositionMapAdapter::checkCapacity(ulong block)
	{
		if (block >= this->capacity)
		{
			throw boost::format("block %1% out of bound (capacity %2%)") % block % this->capacity;
		}
	}
}
