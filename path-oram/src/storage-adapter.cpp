#include "storage-adapter.hpp"

#include <boost/format.hpp>
#include <vector>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsStorageAdapter::~AbsStorageAdapter(){};

	pair<ulong, bytes> AbsStorageAdapter::get(ulong location)
	{
		this->checkCapacity(location);

		auto raw = this->getInternal(location);

		uchar buffer[raw.size()];

		copy(raw.begin(), raw.end(), buffer);

		ulong id   = ((ulong *)buffer)[0];
		bytes data = bytes(buffer + sizeof(ulong), buffer + sizeof(buffer));

		return {id, data};
	}

	void AbsStorageAdapter::set(ulong location, pair<ulong, bytes> data)
	{
		ulong buffer[1] = {data.first};
		bytes id((uchar *)buffer, (uchar *)buffer + sizeof(ulong));

		bytes raw;
		raw.reserve(sizeof(ulong) + data.second.size());
		raw.insert(raw.end(), id.begin(), id.end());
		raw.insert(raw.end(), data.second.begin(), data.second.end());

		this->checkCapacity(location);
		this->checkBlockSize(raw.size());

		if (raw.size() < this->blockSize)
		{
			raw.resize(this->blockSize, 0x00);
		}

		this->setInternal(location, raw);
	}

	void AbsStorageAdapter::checkCapacity(ulong location)
	{
		if (location >= this->capacity)
		{
			throw boost::format("id %1% out of bound (capacity %2%)") % location % this->capacity;
		}
	}

	void AbsStorageAdapter::checkBlockSize(ulong dataLength)
	{
		if (dataLength > this->blockSize)
		{
			throw boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % this->blockSize;
		}
	}

	AbsStorageAdapter::AbsStorageAdapter(ulong capacity, ulong blockSize) :
		capacity(capacity),
		blockSize(blockSize)
	{
	}

	InMemoryStorageAdapter::~InMemoryStorageAdapter()
	{
		for (ulong i = 0; i < this->capacity; i++)
		{
			delete[] this->blocks[i];
		}
		delete[] this->blocks;
	}

	InMemoryStorageAdapter::InMemoryStorageAdapter(ulong capacity, ulong blockSize) :
		AbsStorageAdapter(capacity, blockSize)
	{
		this->blocks = new uchar *[capacity];
		for (ulong i = 0; i < capacity; i++)
		{
			this->blocks[i] = new uchar[blockSize];
		}

		for (ulong i = 0; i < capacity; i++)
		{
			this->set(i, {ULONG_MAX, bytes()});
		}
	}

	bytes InMemoryStorageAdapter::getInternal(ulong location)
	{
		return bytes(this->blocks[location], this->blocks[location] + this->blockSize);
	}

	void InMemoryStorageAdapter::setInternal(ulong location, bytes raw)
	{
		copy(raw.begin(), raw.end(), this->blocks[location]);
	}
}