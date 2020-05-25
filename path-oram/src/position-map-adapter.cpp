#include "position-map-adapter.hpp"

#include <boost/format.hpp>
#include <cstring>
#include <fstream>

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
		map(new number[capacity]),
		capacity(capacity)
	{
	}

	number InMemoryPositionMapAdapter::get(const number block) const
	{
		checkCapacity(block);

		return map[block];
	}

	void InMemoryPositionMapAdapter::set(const number block, const number leaf)
	{
		checkCapacity(block);

		map[block] = leaf;
	}

	void InMemoryPositionMapAdapter::storeToFile(const string filename) const
	{
		fstream file;

		file.open(filename, fstream::out | fstream::binary | fstream::trunc);
		if (!file)
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		file.seekg(0, file.beg);
		file.write((const char *)map, capacity * sizeof(number));
		file.close();
	}

	void InMemoryPositionMapAdapter::loadFromFile(const string filename)
	{
		fstream file;

		file.open(filename, fstream::in | fstream::binary);
		if (!file)
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		file.seekg(0, file.beg);
		file.read((char *)map, capacity * sizeof(number));
		file.close();
	}

	void InMemoryPositionMapAdapter::checkCapacity(const number block) const
	{
#if INPUT_CHECKS
		if (block >= capacity)
		{
			throw Exception(boost::format("block %1% out of bound (capacity %2%)") % block % capacity);
		}
#endif
	}

	ORAMPositionMapAdapter::~ORAMPositionMapAdapter()
	{
	}

	ORAMPositionMapAdapter::ORAMPositionMapAdapter(const shared_ptr<ORAM> oram) :
		oram(oram)
	{
	}

	number ORAMPositionMapAdapter::get(const number block) const
	{
		bytes returned;
		oram->get(block, returned);
		uchar buffer[returned.size()];
		copy(returned.begin(), returned.end(), buffer);

		return ((number *)buffer)[0];
	}

	void ORAMPositionMapAdapter::set(const number block, const number leaf)
	{
		number buffer[1];
		buffer[0] = leaf;

		auto data = bytes((uchar *)buffer, (uchar *)buffer + sizeof(number));

		oram->put(block, data);
	}
}
