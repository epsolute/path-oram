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

	ORAMPositionMapAdapter::~ORAMPositionMapAdapter()
	{
	}

	ORAMPositionMapAdapter::ORAMPositionMapAdapter(ORAM *oram) :
		oram(oram)
	{
	}

	ulong ORAMPositionMapAdapter::get(ulong block)
	{
		auto returned = this->oram->get(block);
		uchar buffer[returned.size()];
		copy(returned.begin(), returned.end(), buffer);

		return ((ulong *)buffer)[0];
	}

	void ORAMPositionMapAdapter::set(ulong block, ulong leaf)
	{
		ulong buffer[1];
		buffer[0] = leaf;

		auto data = bytes((uchar *)buffer, (uchar *)buffer + sizeof(ulong));

		this->oram->put(block, data);
	}
}
