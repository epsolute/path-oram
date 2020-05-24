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

#define RECORD_AND_EXECUTE(condition, plain, record)                                                            \
	if (condition)                                                                                              \
	{                                                                                                           \
		plain;                                                                                                  \
	}                                                                                                           \
	else                                                                                                        \
	{                                                                                                           \
		auto start = chrono::steady_clock::now();                                                               \
		plain;                                                                                                  \
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

	void AbsStorageAdapter::get(const vector<number> &locations, vector<block> &response) const
	{
		for (auto &&location : locations)
		{
			checkCapacity(location);
		}

		// optimize for single operation
		vector<bytes> raws;
		raws.reserve(locations.size());

		if (locations.size() == 1)
		{
			raws.resize(1);
			getAndRecord(locations[0], raws[0]);
		}
		else
		{
			getAndRecord(locations, raws);
		}

		response.reserve(locations.size() * Z);
		for (auto &&raw : raws)
		{
			// decompose to ID and cipher

			bytes decrypted;
			encrypt(
				key.begin(),
				key.end(),
				raw.begin(),
				raw.begin() + AES_BLOCK_SIZE,
				raw.begin() + AES_BLOCK_SIZE,
				raw.end(),
				decrypted,
				DECRYPT);

			const auto length = decrypted.size() / Z;

			for (auto i = 0uLL; i < Z; i++)
			{
				// decompose to ID and data (extract ID from bytes)
				uchar buffer[AES_BLOCK_SIZE];
				copy(decrypted.begin() + i * length, decrypted.begin() + i * length + AES_BLOCK_SIZE, buffer);

				response.push_back(
					{((number *)buffer)[0],
					 bytes(decrypted.begin() + i * length + AES_BLOCK_SIZE, decrypted.begin() + (i + 1) * length)});
			}
		}
	}

	void AbsStorageAdapter::set(const request_anyrange requests)
	{
		vector<block> writes;

		for (auto &&[location, blocks] : requests)
		{
			checkCapacity(location);

#if INPUT_CHECKS
			if (blocks.size() != Z)
			{
				throw Exception(boost::format("each set request must contain exactly Z=%1% blocks (%2% given)") % Z % blocks.size());
			}
#endif

			bytes toEncrypt;
			toEncrypt.reserve(AES_BLOCK_SIZE + userBlockSize * Z);

			for (auto &&block : blocks)
			{
				checkBlockSize(block.second.size());

				// pad if necessary
				if (block.second.size() < userBlockSize)
				{
					block.second.resize(userBlockSize, 0x00);
				}

				// represent ID as a vector of bytes of length AES_BLOCK_SIZE
				const number buffer[1] = {block.first};
				bytes id((uchar *)buffer, (uchar *)buffer + sizeof(number));
				id.resize(AES_BLOCK_SIZE, 0x00);

				// merge ID and data
				toEncrypt.insert(toEncrypt.end(), id.begin(), id.end());
				toEncrypt.insert(toEncrypt.end(), block.second.begin(), block.second.end());
			}

			auto iv = getRandomBlock(AES_BLOCK_SIZE);
			bytes encrypted;
			encrypt(
				key.begin(),
				key.end(),
				iv.begin(),
				iv.end(),
				toEncrypt.begin(),
				toEncrypt.end(),
				iv, // append result to IV
				ENCRYPT);

			writes.push_back({location, iv});
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

	void AbsStorageAdapter::get(const number location, bucket &response) const
	{
		const auto locations = vector<number>{location};
		get(locations, response);
	}

	void AbsStorageAdapter::set(const number location, const bucket &data)
	{
		const auto requests = vector<pair<const number, vector<block>>>{{location, data}};

		set(boost::make_iterator_range(requests.begin(), requests.end()));
	}

	void AbsStorageAdapter::getInternal(const vector<number> &locations, vector<bytes> &response) const
	{
		response.resize(locations.size());
		for (unsigned int i = 0; i < locations.size(); i++)
		{
			getAndRecord(locations[i], response[i]);
		}
	}

	void AbsStorageAdapter::setInternal(const vector<block> &requests)
	{
		for (auto &&request : requests)
		{
			setAndRecord(request.first, request.second);
		}
	}

	void AbsStorageAdapter::checkCapacity(const number location) const
	{
#if INPUT_CHECKS
		if (location >= capacity)
		{
			throw Exception(boost::format("id %1% out of bound (capacity %2%)") % location % capacity);
		}
#endif
	}

	void AbsStorageAdapter::checkBlockSize(const number dataLength) const
	{
#if INPUT_CHECKS
		if (dataLength > userBlockSize)
		{
			throw Exception(boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % userBlockSize);
		}
#endif
	}

	AbsStorageAdapter::AbsStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const number Z) :
		key(key.size() == KEYSIZE ? key : getRandomBlock(KEYSIZE)),
		Z(Z),
		capacity(capacity),
		blockSize((userBlockSize + AES_BLOCK_SIZE) * Z + AES_BLOCK_SIZE), // IV + Z * (ID + PAYLOAD)
		userBlockSize(userBlockSize)
	{
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

	void AbsStorageAdapter::setAndRecord(const number location, const bytes &raw)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty(),
			setInternal(location, raw),
			onStorageRequest(false, 1, raw.size(), elapsed));
	}

	void AbsStorageAdapter::getAndRecord(const number location, bytes &response) const
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty(),
			getInternal(location, response),
			onStorageRequest(true, 1, response.size(), elapsed));
	}

	void AbsStorageAdapter::setAndRecord(const vector<pair<number, bytes>> &requests)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty() || !supportsBatchSet(),
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

	void AbsStorageAdapter::getAndRecord(const vector<number> &locations, vector<bytes> &response) const
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty() || !supportsBatchGet(),
			getInternal(locations, response),
			{
				auto size = 0;
				for (auto &&raw : response)
				{
					size += raw.size();
				}
				onStorageRequest(true, response.size(), size, elapsed);
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

	InMemoryStorageAdapter::InMemoryStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const number Z) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z),
		blocks(new uchar *[capacity])
	{
		for (auto i = 0uLL; i < capacity; i++)
		{
			blocks[i] = new uchar[blockSize];
		}

		fillWithZeroes();
	}

	void InMemoryStorageAdapter::getInternal(const number location, bytes &response) const
	{
		response.insert(response.begin(), blocks[location], blocks[location] + blockSize);
	}

	void InMemoryStorageAdapter::setInternal(const number location, const bytes &raw)
	{
		copy(raw.begin(), raw.end(), blocks[location]);
	}

#pragma endregion InMemoryStorageAdapter

#pragma region FileSystemStorageAdapter

	FileSystemStorageAdapter::~FileSystemStorageAdapter()
	{
		file->close();
	}

	FileSystemStorageAdapter::FileSystemStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const string filename, const bool override, const number Z) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z),
		file(make_unique<fstream>())
	{
		auto flags = fstream::in | fstream::out | fstream::binary;
		if (override)
		{
			flags |= fstream::trunc;
		}

		file->open(filename, flags);
		if (!file->is_open())
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		if (override)
		{
			file->seekg(0, file->beg);
			uchar placeholder[blockSize];

			for (number i = 0; i < capacity; i++)
			{
				file->write((const char *)placeholder, blockSize);
			}

			fillWithZeroes();
		}
	}

	void FileSystemStorageAdapter::getInternal(const number location, bytes &response) const
	{
		uchar placeholder[blockSize];
		file->seekg(location * blockSize, file->beg);
		file->read((char *)placeholder, blockSize);

		response.insert(response.begin(), placeholder, placeholder + blockSize);
	}

	void FileSystemStorageAdapter::setInternal(const number location, const bytes &raw)
	{
		uchar placeholder[blockSize];
		copy(raw.begin(), raw.end(), placeholder);

		file->seekp(location * blockSize, file->beg);
		file->write((const char *)placeholder, blockSize);
	}

#pragma endregion FileSystemStorageAdapter

#if USE_REDIS
#pragma region RedisStorageAdapter

	RedisStorageAdapter::~RedisStorageAdapter()
	{
	}

	RedisStorageAdapter::RedisStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const string host, const bool override, const number Z) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z),
		redis(make_unique<sw::redis::Redis>(host))
	{
		redis->ping();

		if (override)
		{
			redis->flushdb();

			fillWithZeroes();
		}
	}

	void RedisStorageAdapter::getInternal(const number location, bytes &response) const
	{
		const auto rawStr = redis->get(to_string(location));
		response.insert(response.begin(), rawStr.value().begin(), rawStr.value().end());
	}

	void RedisStorageAdapter::setInternal(const number location, const bytes &raw)
	{
		redis->set(to_string(location), string(raw.begin(), raw.end()));
	}

	void RedisStorageAdapter::setInternal(const vector<pair<number, bytes>> &requests)
	{
		vector<pair<string, string>> input;
		input.resize(requests.size());
		transform(requests.begin(), requests.end(), input.begin(), [](block val) { return make_pair<string, string>(to_string(val.first), string(val.second.begin(), val.second.end())); });
		redis->mset(input.begin(), input.end());
	}

	void RedisStorageAdapter::getInternal(const vector<number> &locations, vector<bytes> &response) const
	{
		vector<string> input;
		input.resize(locations.size());
		transform(locations.begin(), locations.end(), input.begin(), [](number val) { return to_string(val); });

		vector<optional<string>> returned;
		redis->mget(input.begin(), input.end(), back_inserter(returned));

		response.resize(returned.size());
		transform(returned.begin(), returned.end(), response.begin(), [](optional<string> val) { return bytes(val.value().begin(), val.value().end()); });
	}

#pragma endregion RedisStorageAdapter
#endif

#if USE_AEROSPIKE
#pragma region AerospikeStorageAdapter

	AerospikeStorageAdapter::~AerospikeStorageAdapter()
	{
		as_error err;

		aerospike_close(as.get(), &err);
		aerospike_destroy(as.get());
	}

	AerospikeStorageAdapter::AerospikeStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const string host, const bool override, const number Z, const string asset) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z),
		as(make_unique<aerospike>()),
		asset(asset)
	{
		as_config config;
		as_config_init(&config);
		as_config_add_host(&config, host.c_str(), 3000);

		aerospike_init(as.get(), &config);

		as_error err;

		aerospike_connect(as.get(), &err);

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

	void AerospikeStorageAdapter::getInternal(const number location, bytes &response) const
	{
		as_key asKey;
		as_key_init(&asKey, "test", asset.c_str(), to_string(location).c_str());

		as_error err;
		as_record *p_rec = NULL;

		aerospike_key_get(as.get(), &err, NULL, &asKey, &p_rec);

		as_bytes *rawBytes = as_record_get_bytes(p_rec, "value");

		uint8_t *rawChars = as_bytes_get(rawBytes);

		response.insert(response.begin(), rawChars, rawChars + blockSize);

		as_record_destroy(p_rec);
		p_rec = NULL;
	}

	void AerospikeStorageAdapter::setInternal(const number location, const bytes &raw)
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

		aerospike_key_put(as.get(), &err, NULL, &asKey, &rec);
	}

	void AerospikeStorageAdapter::getInternal(const vector<number> &locations, vector<bytes> &response) const
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
		aerospike_batch_get(as.get(), &err, NULL, &batch, callback, &udata);

		as_batch_destroy(&batch);

		for (uint i = 0; i < locations.size(); i++)
		{
			delete[] strs[i];
		}

		copy(udata.result.begin(), udata.result.end(), back_inserter(response));
	}

	void AerospikeStorageAdapter::deleteAll()
	{
		as_error err;
		aerospike_truncate(as.get(), &err, NULL, "test", asset.c_str(), 0);
	}

#pragma endregion AerospikeStorageAdapter
#endif
}
