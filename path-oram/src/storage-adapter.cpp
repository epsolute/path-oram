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
		for (int i = 0; i < this->capacity; i++)
		{
			delete[] this->blocks[i];
		}
		delete[] this->blocks;
	}

	InMemoryStorageAdapter::InMemoryStorageAdapter(unsigned int capacity, unsigned int blockSize) :
		capacity(capacity),
		blockSize(blockSize)
	{
		this->blocks = new unsigned char*[capacity];
		for (int i = 0; i < capacity; i++)
		{
			this->blocks[i] = new unsigned char[blockSize];
		}
	}

	vector<unsigned char> InMemoryStorageAdapter::get(unsigned long id)
	{
		this->checkCapacity(id);

		return vector<unsigned char>(this->blocks[id], this->blocks[id] + this->blockSize);
	}

	void InMemoryStorageAdapter::set(unsigned long id, vector<unsigned char> data)
	{
		this->checkCapacity(id);
		this->checkBlockSize(data.size());

		if (data.size() < this->blockSize)
		{
			data.resize(this->blockSize, 0x00);
		}

		copy(data.begin(), data.end(), this->blocks[id]);
	}

	void InMemoryStorageAdapter::checkCapacity(unsigned int id)
	{
		if (id >= this->capacity)
		{
			throw boost::format("id %1% out of bound (capacity %2%)") % id % this->capacity;
		}
	}

	void InMemoryStorageAdapter::checkBlockSize(unsigned int dataLength)
	{
		if (dataLength > this->blockSize)
		{
			throw boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % this->blockSize;
		}
	}
}
