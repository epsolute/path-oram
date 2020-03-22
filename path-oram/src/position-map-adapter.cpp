#include "position-map-adapter.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsPositionMapAdapeter::~AbsPositionMapAdapeter(){};

	InMemoryPositionMapAdapeter::~InMemoryPositionMapAdapeter()
	{
		delete[] this->map;
	}

	InMemoryPositionMapAdapeter::InMemoryPositionMapAdapeter(unsigned int capacity) :
		capacity(capacity)
	{
		this->map = new unsigned long[capacity];
	}

	unsigned long InMemoryPositionMapAdapeter::get(unsigned long block)
	{
		this->checkCapacity(block);

		return this->map[block];
	}

	void InMemoryPositionMapAdapeter::set(unsigned long block, unsigned long leaf)
	{
		this->checkCapacity(block);

		this->map[block] = leaf;
	}

	void InMemoryPositionMapAdapeter::checkCapacity(unsigned int block)
	{
		if (block >= this->capacity)
		{
			throw boost::format("block %1% out of bound (capacity %2%)") % block % this->capacity;
		}
	}
}
