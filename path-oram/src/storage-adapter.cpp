#include "storage-adapter.hpp"

#include <aerospike/aerospike_batch.h>
#include <aerospike/aerospike_key.h>
#include <aerospike/as_batch.h>
#include <aerospike/as_key.h>
#include <aerospike/as_record.h>
#include <boost/format.hpp>
#include <cstring>
#include <openssl/aes.h>
#include <utility.hpp>
#include <vector>

#define RECORD_AND_EXECUTE(condition, plain, execute, record)                                                   \
	if (condition)                                                                                              \
	{                                                                                                           \
		plain;                                                                                                  \
	}                                                                                                           \
	else                                                                                                        \
	{                                                                                                           \
		auto start = chrono::steady_clock::now();                                                               \
		execute;                                                                                                \
		auto elapsed = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now() - start).count(); \
		record;                                                                                                 \
	}

namespace PathORAM
{
	using namespace std;
	using boost::format;

#pragma region AbsStorageAdapter

	AbsStorageAdapter::~AbsStorageAdapter()
	{
	}

	vector<block> AbsStorageAdapter::get(vector<number> locations)
	{
		for (auto location : locations)
		{
			checkCapacity(location);
		}

		// optimize for single operation
		vector<bytes> raws;
		if (locations.size() == 1)
		{
			raws.push_back(getAndRecord(locations[0]));
		}
		else
		{
			raws = getAndRecord(locations);
		}

		vector<block> results;
		results.reserve(raws.size() * Z);

		for (auto raw : raws)
		{
			// decompose to ID and cipher
			bytes iv(raw.begin(), raw.begin() + AES_BLOCK_SIZE);
			bytes ciphertext(raw.begin() + AES_BLOCK_SIZE, raw.end());

			// decryption
			auto decrypted = encrypt(key, iv, ciphertext, DECRYPT);
			auto length	   = decrypted.size() / Z;

			for (auto i = 0uLL; i < Z; i++)
			{
				// decompose to ID and data
				bytes idBytes(decrypted.begin() + i * length, decrypted.begin() + i * length + AES_BLOCK_SIZE);
				bytes data(decrypted.begin() + i * length + AES_BLOCK_SIZE, decrypted.begin() + (i + 1) * length);

				// extract ID from bytes
				uchar buffer[idBytes.size()];
				copy(idBytes.begin(), idBytes.end(), buffer);
				auto id = ((number *)buffer)[0];

				results.push_back({id, data});
			}
		}

		return results;
	}

	void AbsStorageAdapter::set(vector<pair<number, bucket>> requests)
	{
		vector<block> writes;

		for (auto [location, blocks] : requests)
		{
			checkCapacity(location);

			if (blocks.size() != Z)
			{
				throw Exception(boost::format("each set request must contain exactly Z=%1% blocks (%2% given)") % Z % blocks.size());
			}

			bytes toEncrypt;
			toEncrypt.reserve(AES_BLOCK_SIZE + userBlockSize * Z);

			for (auto block : blocks)
			{
				checkBlockSize(block.second.size());

				// pad if necessary
				if (block.second.size() < userBlockSize)
				{
					block.second.resize(userBlockSize, 0x00);
				}

				// represent ID as a vector of bytes of length AES_BLOCK_SIZE
				number buffer[1] = {block.first};
				bytes id((uchar *)buffer, (uchar *)buffer + sizeof(number));
				id.resize(AES_BLOCK_SIZE, 0x00);

				// merge ID and data
				toEncrypt.insert(toEncrypt.end(), id.begin(), id.end());
				toEncrypt.insert(toEncrypt.end(), block.second.begin(), block.second.end());
			}

			// encryption
			auto iv		   = getRandomBlock(AES_BLOCK_SIZE);
			auto encrypted = encrypt(key, iv, toEncrypt, ENCRYPT);

			// append IV
			bytes raw;
			raw.reserve(iv.size() + encrypted.size());
			raw.insert(raw.end(), iv.begin(), iv.end());
			raw.insert(raw.end(), encrypted.begin(), encrypted.end());

			writes.push_back({location, raw});
		}

		// optimize for single operation
		if (writes.size() == 1)
		{
			setAndRecord(writes[0].first, writes[0].second);
		}
		else
		{
			setAndRecord(writes);
		}
	}

	bucket AbsStorageAdapter::get(number location)
	{
		return get(vector<number>{location});
	}

	void AbsStorageAdapter::set(number location, bucket data)
	{
		set(vector<pair<number, vector<block>>>{{location, data}});
	}

	vector<bytes> AbsStorageAdapter::getInternal(vector<number> locations)
	{
		vector<bytes> result;
		result.resize(locations.size());
		for (unsigned int i = 0; i < locations.size(); i++)
		{
			result[i] = getAndRecord(locations[i]);
		}

		return result;
	}

	void AbsStorageAdapter::setInternal(vector<block> requests)
	{
		for (auto request : requests)
		{
			setAndRecord(request.first, request.second);
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

	AbsStorageAdapter::AbsStorageAdapter(number capacity, number userBlockSize, bytes key, number Z) :
		key(key),
		Z(Z),
		capacity(capacity),
		blockSize((userBlockSize + AES_BLOCK_SIZE) * Z + AES_BLOCK_SIZE), // IV + Z * (ID + PAYLOAD)
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

		if (Z == 0)
		{
			throw Exception(boost::format("Z must be greater than zero (provided %1%)") % Z);
		}
	}

	void AbsStorageAdapter::fillWithZeroes()
	{
		for (auto i = 0uLL; i < capacity; i++)
		{
			bucket bucket;
			for (auto j = 0uLL; j < Z; j++)
			{
				bucket.push_back({ULONG_MAX, bytes()});
			}

			set(i, bucket);
		}
	}

	boost::signals2::connection AbsStorageAdapter::subscribe(const OnStorageRequest::slot_type &handler)
	{
		return onStorageRequest.connect(handler);
	}

	void AbsStorageAdapter::setAndRecord(number location, bytes raw)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty(),
			setInternal(location, raw),
			setInternal(location, raw),
			onStorageRequest(false, 1, raw.size(), elapsed));
	}

	bytes AbsStorageAdapter::getAndRecord(number location)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty(),
			return getInternal(location),
			auto raw = getInternal(location),
			{
				onStorageRequest(true, 1, raw.size(), elapsed);
				return raw;
			});
	}

	void AbsStorageAdapter::setAndRecord(vector<pair<number, bytes>> requests)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty() || !supportsBatchSet(),
			setInternal(requests),
			setInternal(requests),
			{
				auto size = 0;
				for (auto &&request : requests)
				{
					size += request.second.size();
				}
				onStorageRequest(false, requests.size(), size, elapsed);
			});
	}

	vector<bytes> AbsStorageAdapter::getAndRecord(vector<number> locations)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty() || !supportsBatchGet(),
			return getInternal(locations),
			auto raws = getInternal(locations),
			{
				auto size = 0;
				for (auto &&raw : raws)
				{
					size += raw.size();
				}
				onStorageRequest(true, raws.size(), size, elapsed);
				return raws;
			});
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

	InMemoryStorageAdapter::InMemoryStorageAdapter(number capacity, number userBlockSize, bytes key, number Z) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z)
	{
		this->blocks = new uchar *[capacity];
		for (auto i = 0uLL; i < capacity; i++)
		{
			blocks[i] = new uchar[blockSize];
		}

		fillWithZeroes();
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

	FileSystemStorageAdapter::FileSystemStorageAdapter(number capacity, number userBlockSize, bytes key, string filename, bool override, number Z) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z)
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

			fillWithZeroes();
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

	RedisStorageAdapter::RedisStorageAdapter(number capacity, number userBlockSize, bytes key, string host, bool override, number Z) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z)
	{
		redis = make_unique<sw::redis::Redis>(host);
		redis->ping();

		if (override)
		{
			redis->flushdb();

			fillWithZeroes();
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

	void RedisStorageAdapter::setInternal(vector<pair<number, bytes>> requests)
	{
		vector<pair<string, string>> input;
		input.resize(requests.size());
		transform(requests.begin(), requests.end(), input.begin(), [](block val) { return make_pair<string, string>(to_string(val.first), string(val.second.begin(), val.second.end())); });
		redis->mset(input.begin(), input.end());
	}

	vector<bytes> RedisStorageAdapter::getInternal(vector<number> locations)
	{
		vector<string> input;
		input.resize(locations.size());
		transform(locations.begin(), locations.end(), input.begin(), [](number val) { return to_string(val); });

		vector<optional<string>> returned;
		redis->mget(input.begin(), input.end(), back_inserter(returned));

		vector<bytes> results;
		results.resize(returned.size());
		transform(returned.begin(), returned.end(), results.begin(), [](optional<string> val) { return bytes(val.value().begin(), val.value().end()); });

		return results;
	}

#pragma endregion RedisStorageAdapter

#pragma region AerospikeStorageAdapter

	AerospikeStorageAdapter::~AerospikeStorageAdapter()
	{
		as_error err;

		aerospike_close(&as, &err);
		aerospike_destroy(&as);
	}

	AerospikeStorageAdapter::AerospikeStorageAdapter(number capacity, number userBlockSize, bytes key, string host, bool override, number Z, string asset) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z), asset(asset)
	{
		as_config config;
		as_config_init(&config);
		as_config_add_host(&config, host.c_str(), 3000);

		aerospike_init(&as, &config);

		as_error err;

		aerospike_connect(&as, &err);

		if (err.code != AEROSPIKE_OK)
		{
			throw Exception(boost::format("connection to Aerospike failed with host: %1% (message: %2%") % host % err.message);
		}

		if (override)
		{
			deleteAll();

			fillWithZeroes();
		}
	}

	bytes AerospikeStorageAdapter::getInternal(number location)
	{
		as_key asKey;
		as_key_init(&asKey, "test", asset.c_str(), to_string(location).c_str());

		as_error err;
		as_record *p_rec = NULL;

		aerospike_key_get(&as, &err, NULL, &asKey, &p_rec);

		as_bytes *rawBytes = as_record_get_bytes(p_rec, "value");

		uint8_t *rawChars = as_bytes_get(rawBytes);

		auto result = bytes(rawChars, rawChars + blockSize);

		as_record_destroy(p_rec);
		p_rec = NULL;

		return result;
	}

	void AerospikeStorageAdapter::setInternal(number location, bytes raw)
	{
		as_key asKey;
		as_key_init(&asKey, "test", asset.c_str(), to_string(location).c_str());

		uint8_t rawChars[raw.size()];
		copy(raw.begin(), raw.end(), rawChars);
		as_bytes rawBytes;
		as_bytes_init_wrap(&rawBytes, rawChars, raw.size(), false);

		as_record rec;
		as_record_inita(&rec, 1);
		as_record_set_bytes(&rec, "value", &rawBytes);

		as_error err;

		aerospike_key_put(&as, &err, NULL, &asKey, &rec);
	}

	vector<bytes> AerospikeStorageAdapter::getInternal(vector<number> locations)
	{
		as_batch batch;
		as_batch_inita(&batch, locations.size());

		char *strs[locations.size()];

		for (uint i = 0; i < locations.size(); i++)
		{
			strs[i] = new char[to_string(locations[i]).length() + 1];
			strcpy(strs[i], to_string(locations[i]).c_str());
			as_key_init(as_batch_keyat(&batch, i), "test", asset.c_str(), strs[i]);
		}

		struct UData
		{
			vector<bytes> result;
			number blockSize;
		};

		auto callback = [](const as_batch_read *results, uint32_t n, void *udata) -> bool {
			for (uint i = 0; i < n; i++)
			{
				as_bytes *rawBytes = as_record_get_bytes(&results[i].record, "value");

				uint8_t *rawChars = as_bytes_get(rawBytes);

				auto t						= bytes(rawChars, rawChars + ((UData *)udata)->blockSize);
				((UData *)udata)->result[i] = t;
			}

			return true;
		};

		UData udata{
			.result	   = vector<bytes>(),
			.blockSize = blockSize};
		udata.result.resize(locations.size());

		as_error err;
		aerospike_batch_get(&as, &err, NULL, &batch, callback, &udata);

		as_batch_destroy(&batch);

		for (uint i = 0; i < locations.size(); i++)
		{
			delete[] strs[i];
		}

		return udata.result;
	}

	void AerospikeStorageAdapter::deleteAll()
	{
		as_error err;
		aerospike_truncate(&as, &err, NULL, "test", asset.c_str(), 0);
	}

#pragma endregion AerospikeStorageAdapter
}
