#include "storage-adapter.hpp"

#include "utility.hpp"

#include <boost/format.hpp>
#include <cstring>
#include <openssl/aes.h>
#include <vector>

namespace PathORAM
{
	using namespace std;
	using boost::format;

#pragma region AbsStorageAdapter

	AbsStorageAdapter::~AbsStorageAdapter(){};

	pair<number, bytes> AbsStorageAdapter::get(number location)
	{
		this->checkCapacity(location);

		auto raw = this->getInternal(location);

		uchar buffer[raw.size()];

		copy(raw.begin(), raw.end(), buffer);

		auto id   = ((number *)buffer)[0];
		auto iv   = bytes(buffer + sizeof(number), buffer + sizeof(number) + AES_BLOCK_SIZE);
		auto data = bytes(buffer + sizeof(number) + AES_BLOCK_SIZE, buffer + sizeof(buffer));

		// decryption
		auto decrypted = encrypt(this->key, iv, data, DECRYPT);

		return {id, decrypted};
	}

	void AbsStorageAdapter::set(number location, pair<number, bytes> data)
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

		number buffer[1] = {data.first};
		bytes id((uchar *)buffer, (uchar *)buffer + sizeof(number));

		bytes raw;
		raw.reserve(sizeof(number) + iv.size() + encrypted.size());
		raw.insert(raw.end(), id.begin(), id.end());
		raw.insert(raw.end(), iv.begin(), iv.end());
		raw.insert(raw.end(), encrypted.begin(), encrypted.end());

		this->setInternal(location, raw);
	}

	void AbsStorageAdapter::checkCapacity(number location)
	{
		if (location >= this->capacity)
		{
			throw boost::str(boost::format("id %1% out of bound (capacity %2%)") % location % this->capacity);
		}
	}

	void AbsStorageAdapter::checkBlockSize(number dataLength)
	{
		if (dataLength > this->userBlockSize)
		{
			throw boost::str(boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % this->userBlockSize);
		}
	}

	AbsStorageAdapter::AbsStorageAdapter(number capacity, number userBlockSize, bytes key) :
		key(key),
		capacity(capacity),
		blockSize(userBlockSize + sizeof(number) + AES_BLOCK_SIZE),
		userBlockSize(userBlockSize)
	{
		if (key.size() != KEYSIZE)
		{
			this->key = getRandomBlock(KEYSIZE);
		}

		if (userBlockSize < 2 * AES_BLOCK_SIZE)
		{
			throw boost::str(boost::format("block size %1% is too small, need ata least %2%") % userBlockSize % (2 * AES_BLOCK_SIZE));
		}

		if (userBlockSize % AES_BLOCK_SIZE != 0)
		{
			throw boost::str(boost::format("block size must be a multiple of %1% (provided %2% bytes)") % AES_BLOCK_SIZE % userBlockSize);
		}
	}

#pragma endregion AbsStorageAdapter

#pragma region InMemoryStorageAdapter

	InMemoryStorageAdapter::~InMemoryStorageAdapter()
	{
		for (number i = 0; i < this->capacity; i++)
		{
			delete[] this->blocks[i];
		}
		delete[] this->blocks;
	}

	InMemoryStorageAdapter::InMemoryStorageAdapter(number capacity, number userBlockSize, bytes key) :
		AbsStorageAdapter(capacity, userBlockSize, key)
	{
		this->blocks = new uchar *[capacity];
		for (number i = 0; i < capacity; i++)
		{
			this->blocks[i] = new uchar[this->blockSize];
		}

		for (number i = 0; i < capacity; i++)
		{
			this->set(i, {ULONG_MAX, bytes()});
		}
	}

	bytes InMemoryStorageAdapter::getInternal(number location)
	{
		return bytes(this->blocks[location], this->blocks[location] + this->blockSize);
	}

	void InMemoryStorageAdapter::setInternal(number location, bytes raw)
	{
		copy(raw.begin(), raw.end(), this->blocks[location]);
	}

#pragma endregion InMemoryStorageAdapter

#pragma region FileSystemStorageAdapter

	FileSystemStorageAdapter::~FileSystemStorageAdapter()
	{
		this->file.close();
	}

	FileSystemStorageAdapter::FileSystemStorageAdapter(number capacity, number userBlockSize, bytes key, string filename, bool override) :
		AbsStorageAdapter(capacity, userBlockSize, key)
	{
		auto flags = fstream::in | fstream::out | fstream::binary;
		if (override)
		{
			flags |= fstream::trunc;
		}

		this->file.open(filename, flags);
		if (!this->file)
		{
			throw boost::str(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		if (override)
		{
			this->file.seekg(0, this->file.beg);
			uchar placeholder[this->blockSize];

			for (number i = 0; i < capacity; i++)
			{
				this->file.write((const char *)placeholder, this->blockSize);
			}

			for (number i = 0; i < capacity; i++)
			{
				this->set(i, {ULONG_MAX, bytes()});
			}
		}
	}

	bytes FileSystemStorageAdapter::getInternal(number location)
	{
		uchar placeholder[this->blockSize];
		this->file.seekg(location * this->blockSize, this->file.beg);
		this->file.read((char *)placeholder, this->blockSize);

		return bytes(placeholder, placeholder + this->blockSize);
	}

	void FileSystemStorageAdapter::setInternal(number location, bytes raw)
	{
		uchar placeholder[this->blockSize];
		copy(raw.begin(), raw.end(), placeholder);

		this->file.seekp(location * this->blockSize, this->file.beg);
		this->file.write((const char *)placeholder, this->blockSize);
	}

#pragma endregion FileSystemStorageAdapter
}
