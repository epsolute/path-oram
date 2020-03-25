#include "storage-adapter.hpp"

#include "utility.hpp"

#include <boost/format.hpp>
#include <openssl/aes.h>
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

		auto id   = ((ulong *)buffer)[0];
		auto iv   = bytes(buffer + sizeof(ulong), buffer + sizeof(ulong) + AES_BLOCK_SIZE);
		auto data = bytes(buffer + sizeof(ulong) + AES_BLOCK_SIZE, buffer + sizeof(buffer));

		// decryption
		auto decrypted = encrypt(this->key, iv, data, DECRYPT);

		return {id, decrypted};
	}

	void AbsStorageAdapter::set(ulong location, pair<ulong, bytes> data)
	{
		this->checkCapacity(location);
		this->checkBlockSize(data.second.size());

		if (data.second.size() < this->userBlockSize)
		{
			data.second.resize(this->userBlockSize, 0x00);
		}

		// encryption
		auto iv		   = getRandomBlock(AES_BLOCK_SIZE);
		auto encrypted = encrypt(this->key, iv, data.second, ENCRYPT);

		ulong buffer[1] = {data.first};
		bytes id((uchar *)buffer, (uchar *)buffer + sizeof(ulong));

		bytes raw;
		raw.reserve(sizeof(ulong) + iv.size() + encrypted.size());
		raw.insert(raw.end(), id.begin(), id.end());
		raw.insert(raw.end(), iv.begin(), iv.end());
		raw.insert(raw.end(), encrypted.begin(), encrypted.end());

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
		if (dataLength > this->userBlockSize)
		{
			throw boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % this->userBlockSize;
		}
	}

	AbsStorageAdapter::AbsStorageAdapter(ulong capacity, ulong userBlockSize) :
		key(getRandomBlock(KEYSIZE)),
		capacity(capacity),
		blockSize(userBlockSize + sizeof(ulong) + AES_BLOCK_SIZE),
		userBlockSize(userBlockSize)
	{
		if (userBlockSize < 2 * AES_BLOCK_SIZE)
		{
			throw boost::format("block size %1% is too small, need ata least %2%") % userBlockSize % (2 * AES_BLOCK_SIZE);
		}

		if (userBlockSize % AES_BLOCK_SIZE != 0)
		{
			throw boost::format("block size must be a multiple of %1% (provided %2% bytes)") % AES_BLOCK_SIZE % userBlockSize;
		}
	}

	InMemoryStorageAdapter::~InMemoryStorageAdapter()
	{
		for (ulong i = 0; i < this->capacity; i++)
		{
			delete[] this->blocks[i];
		}
		delete[] this->blocks;
	}

	InMemoryStorageAdapter::InMemoryStorageAdapter(ulong capacity, ulong userBlockSize) :
		AbsStorageAdapter(capacity, userBlockSize)
	{
		this->blocks = new uchar *[capacity];
		for (ulong i = 0; i < capacity; i++)
		{
			this->blocks[i] = new uchar[this->blockSize];
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
