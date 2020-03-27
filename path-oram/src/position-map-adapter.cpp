#include "position-map-adapter.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsPositionMapAdapter::~AbsPositionMapAdapter(){};

	InMemoryPositionMapAdapter::~InMemoryPositionMapAdapter()
	{
		delete[] map;
	}

	InMemoryPositionMapAdapter::InMemoryPositionMapAdapter(number capacity) :
		capacity(capacity)
	{
		this->map = new number[capacity];
	}

	number InMemoryPositionMapAdapter::get(number block)
	{
		checkCapacity(block);

		return map[block];
	}

	void InMemoryPositionMapAdapter::set(number block, number leaf)
	{
		checkCapacity(block);

		map[block] = leaf;
	}

	void InMemoryPositionMapAdapter::checkCapacity(number block)
	{
		if (block >= capacity)
		{
			throw boost::str(boost::format("block %1% out of bound (capacity %2%)") % block % capacity);
		}
	}

	ORAMPositionMapAdapter::~ORAMPositionMapAdapter()
	{
	}

	ORAMPositionMapAdapter::ORAMPositionMapAdapter(ORAM *oram) :
		oram(oram)
	{
	}

	number ORAMPositionMapAdapter::get(number block)
	{
		auto returned = oram->get(block);
		uchar buffer[returned.size()];
		copy(returned.begin(), returned.end(), buffer);

		return ((number *)buffer)[0];
	}

	void ORAMPositionMapAdapter::set(number block, number leaf)
	{
		number buffer[1];
		buffer[0] = leaf;

		auto data = bytes((uchar *)buffer, (uchar *)buffer + sizeof(number));

		oram->put(block, data);
	}
}
