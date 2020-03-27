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
		checkCapacity(location);

		auto raw = getInternal(location);

		uchar buffer[raw.size()];

		copy(raw.begin(), raw.end(), buffer);

		auto id   = ((number *)buffer)[0];
		auto iv   = bytes(buffer + sizeof(number), buffer + sizeof(number) + AES_BLOCK_SIZE);
		auto data = bytes(buffer + sizeof(number) + AES_BLOCK_SIZE, buffer + sizeof(buffer));

		// decryption
		auto decrypted = encrypt(key, iv, data, DECRYPT);

		return {id, decrypted};
	}

	void AbsStorageAdapter::set(number location, pair<number, bytes> data)
	{
		checkCapacity(location);
		checkBlockSize(data.second.size());

		if (data.second.size() < userBlockSize)
		{
			data.second.resize(userBlockSize, 0x00);
		}

		// encryption
		auto iv		   = getRandomBlock(AES_BLOCK_SIZE);
		auto encrypted = encrypt(key, iv, data.second, ENCRYPT);

		number buffer[1] = {data.first};
		bytes id((uchar *)buffer, (uchar *)buffer + sizeof(number));

		bytes raw;
		raw.reserve(sizeof(number) + iv.size() + encrypted.size());
		raw.insert(raw.end(), id.begin(), id.end());
		raw.insert(raw.end(), iv.begin(), iv.end());
		raw.insert(raw.end(), encrypted.begin(), encrypted.end());

		setInternal(location, raw);
	}

	void AbsStorageAdapter::checkCapacity(number location)
	{
		if (location >= capacity)
		{
			throw boost::str(boost::format("id %1% out of bound (capacity %2%)") % location % capacity);
		}
	}

	void AbsStorageAdapter::checkBlockSize(number dataLength)
	{
		if (dataLength > userBlockSize)
		{
			throw boost::str(boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % userBlockSize);
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
		for (number i = 0; i < capacity; i++)
		{
			delete[] blocks[i];
		}
		delete[] blocks;
	}

	InMemoryStorageAdapter::InMemoryStorageAdapter(number capacity, number userBlockSize, bytes key) :
		AbsStorageAdapter(capacity, userBlockSize, key)
	{
		this->blocks = new uchar *[capacity];
		for (number i = 0; i < capacity; i++)
		{
			blocks[i] = new uchar[blockSize];
		}

		for (number i = 0; i < capacity; i++)
		{
			set(i, {ULONG_MAX, bytes()});
		}
	}

	bytes InMemoryStorageAdapter::getInternal(number location)
	{
		return bytes(blocks[location], blocks[location] + blockSize);
	}

	void InMemoryStorageAdapter::setInternal(number location, bytes raw)
	{
		copy(raw.begin(), raw.end(), blocks[location]);
	}

#pragma endregion InMemoryStorageAdapter

#pragma region FileSystemStorageAdapter

	FileSystemStorageAdapter::~FileSystemStorageAdapter()
	{
		file.close();
	}

	FileSystemStorageAdapter::FileSystemStorageAdapter(number capacity, number userBlockSize, bytes key, string filename, bool override) :
		AbsStorageAdapter(capacity, userBlockSize, key)
	{
		auto flags = fstream::in | fstream::out | fstream::binary;
		if (override)
		{
			flags |= fstream::trunc;
		}

		file.open(filename, flags);
		if (!file)
		{
			throw boost::str(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		if (override)
		{
			file.seekg(0, file.beg);
			uchar placeholder[blockSize];

			for (number i = 0; i < capacity; i++)
			{
				file.write((const char *)placeholder, blockSize);
			}

			for (number i = 0; i < capacity; i++)
			{
				set(i, {ULONG_MAX, bytes()});
			}
		}
	}

	bytes FileSystemStorageAdapter::getInternal(number location)
	{
		uchar placeholder[blockSize];
		file.seekg(location * blockSize, file.beg);
		file.read((char *)placeholder, blockSize);

		return bytes(placeholder, placeholder + blockSize);
	}

	void FileSystemStorageAdapter::setInternal(number location, bytes raw)
	{
		uchar placeholder[blockSize];
		copy(raw.begin(), raw.end(), placeholder);

		file.seekp(location * blockSize, file.beg);
		file.write((const char *)placeholder, blockSize);
	}

#pragma endregion FileSystemStorageAdapter
}
