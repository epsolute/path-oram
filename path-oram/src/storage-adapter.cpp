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

	AbsStorageAdapter::~AbsStorageAdapter()
	{
	}

	pair<number, bytes> AbsStorageAdapter::get(number location)
	{
		checkCapacity(location);

		auto raw = getInternal(location);

		// decompose to ID and cipher
		bytes iv(raw.begin(), raw.begin() + AES_BLOCK_SIZE);
		bytes ciphertext(raw.begin() + AES_BLOCK_SIZE, raw.end());

		// decryption
		auto decrypted = encrypt(key, iv, ciphertext, DECRYPT);

		// decompose to ID and data
		bytes idBytes(decrypted.begin(), decrypted.begin() + AES_BLOCK_SIZE);
		bytes data(decrypted.begin() + AES_BLOCK_SIZE, decrypted.end());

		// extract ID from bytes
		uchar buffer[idBytes.size()];
		copy(idBytes.begin(), idBytes.end(), buffer);
		auto id = ((number *)buffer)[0];

		return {id, data};
	}

	void AbsStorageAdapter::set(number location, pair<number, bytes> data)
	{
		checkCapacity(location);
		checkBlockSize(data.second.size());

		// pad if necessary
		if (data.second.size() < userBlockSize)
		{
			data.second.resize(userBlockSize, 0x00);
		}

		// represent ID as a vector of bytes of length AES_BLOCK_SIZE
		number buffer[1] = {data.first};
		bytes id((uchar *)buffer, (uchar *)buffer + sizeof(number));
		id.resize(AES_BLOCK_SIZE, 0x00);

		// merge ID and data
		bytes toEncrypt;
		toEncrypt.reserve(AES_BLOCK_SIZE + userBlockSize);
		toEncrypt.insert(toEncrypt.end(), id.begin(), id.end());
		toEncrypt.insert(toEncrypt.end(), data.second.begin(), data.second.end());

		// encryption
		auto iv		   = getRandomBlock(AES_BLOCK_SIZE);
		auto encrypted = encrypt(key, iv, toEncrypt, ENCRYPT);

		// append IV
		bytes raw;
		raw.reserve(iv.size() + encrypted.size());
		raw.insert(raw.end(), iv.begin(), iv.end());
		raw.insert(raw.end(), encrypted.begin(), encrypted.end());

		setInternal(location, raw);
	}

	vector<pair<number, bytes>> AbsStorageAdapter::get(vector<number> locations)
	{
		vector<pair<number, bytes>> result;
		result.resize(locations.size());
		for (auto i = 0; i < locations.size(); i++)
		{
			result[i] = get(locations[i]);
		}

		return result;
	}

	void AbsStorageAdapter::set(vector<pair<number, pair<number, bytes>>> requests)
	{
		for (auto request : requests)
		{
			set(request.first, request.second);
		}
	}

	void AbsStorageAdapter::checkCapacity(number location)
	{
		if (location >= capacity)
		{
			throw Exception(boost::format("id %1% out of bound (capacity %2%)") % location % capacity);
		}
	}

	void AbsStorageAdapter::checkBlockSize(number dataLength)
	{
		if (dataLength > userBlockSize)
		{
			throw Exception(boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % userBlockSize);
		}
	}

	AbsStorageAdapter::AbsStorageAdapter(number capacity, number userBlockSize, bytes key) :
		key(key),
		capacity(capacity),
		blockSize(userBlockSize + 2 * AES_BLOCK_SIZE), // one block for ID, one for IV
		userBlockSize(userBlockSize)
	{
		if (key.size() != KEYSIZE)
		{
			this->key = getRandomBlock(KEYSIZE);
		}

		if (userBlockSize < 2 * AES_BLOCK_SIZE)
		{
			throw Exception(boost::format("block size %1% is too small, need at least %2%") % userBlockSize % (2 * AES_BLOCK_SIZE));
		}

		if (userBlockSize % AES_BLOCK_SIZE != 0)
		{
			throw Exception(boost::format("block size must be a multiple of %1% (provided %2% bytes)") % AES_BLOCK_SIZE % userBlockSize);
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
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
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

#pragma region RedisStorageAdapter

	RedisStorageAdapter::~RedisStorageAdapter()
	{
	}

	RedisStorageAdapter::RedisStorageAdapter(number capacity, number userBlockSize, bytes key, string host, bool override) :
		AbsStorageAdapter(capacity, userBlockSize, key)
	{
		redis = make_unique<sw::redis::Redis>(host);
		redis->ping();

		if (override)
		{
			redis->flushall();

			for (number i = 0; i < capacity; i++)
			{
				set(i, {ULONG_MAX, bytes()});
			}
		}
	}

	bytes RedisStorageAdapter::getInternal(number location)
	{
		auto rawStr = redis->get(to_string(location));
		return bytes(rawStr.value().begin(), rawStr.value().end());
	}

	void RedisStorageAdapter::setInternal(number location, bytes raw)
	{
		redis->set(to_string(location), string(raw.begin(), raw.end()));
	}

#pragma endregion RedisStorageAdapter
}
