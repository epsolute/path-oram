#include "storage-adapter.hpp"

#include <boost/format.hpp>
#include <vector>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsStorageAdapter::~AbsStorageAdapter(){};

	InMemoryStorageAdapter::~InMemoryStorageAdapter()
	{
		for (ulong i = 0; i < this->capacity; i++)
		{
			delete[] this->blocks[i];
		}
		delete[] this->blocks;
	}

	InMemoryStorageAdapter::InMemoryStorageAdapter(ulong capacity, ulong blockSize) :
		capacity(capacity),
		blockSize(blockSize)
	{
		this->blocks = new unsigned char*[capacity];
		for (ulong i = 0; i < capacity; i++)
		{
			this->blocks[i] = new unsigned char[blockSize];
		}
	}

	bytes InMemoryStorageAdapter::get(ulong id)
	{
		this->checkCapacity(id);

		return bytes(this->blocks[id], this->blocks[id] + this->blockSize);
	}

	void InMemoryStorageAdapter::set(ulong id, bytes data)
	{
		this->checkCapacity(id);
		this->checkBlockSize(data.size());

		if (data.size() < this->blockSize)
		{
			data.resize(this->blockSize, 0x00);
		}

		copy(data.begin(), data.end(), this->blocks[id]);
	}

	void InMemoryStorageAdapter::checkCapacity(ulong id)
	{
		if (id >= this->capacity)
		{
			throw boost::format("id %1% out of bound (capacity %2%)") % id % this->capacity;
		}
	}

	void InMemoryStorageAdapter::checkBlockSize(ulong dataLength)
	{
		if (dataLength > this->blockSize)
		{
			throw boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % this->blockSize;
		}
	}
}
