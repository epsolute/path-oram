#include "stash-adapter.hpp"

#include "utility.hpp"

#include <algorithm>
#include <boost/format.hpp>
#include <cstring>
#include <fstream>
#include <iterator>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsStashAdapter::~AbsStashAdapter() {}

	InMemoryStashAdapter::~InMemoryStashAdapter() {}

	InMemoryStashAdapter::InMemoryStashAdapter(const number capacity) :
		capacity(capacity)
	{
		this->stash = unordered_map<number, bytes>();
		stash.reserve(capacity);
	}

	void InMemoryStashAdapter::getAll(vector<block> &response) const
	{
		response.insert(response.begin(), stash.begin(), stash.end());

		const uint n = response.size();
		if (n >= 2)
		{
			// Fisher-Yates shuffle
			for (uint i = 0; i < n - 1; i++)
			{
				uint j = i + getRandomUInt(n - i);
				swap(response[i], response[j]);
			}
		}
	}

	void InMemoryStashAdapter::add(const number block, const bytes &data)
	{
		checkOverflow(block);

		stash.insert({block, data});
	}

	void InMemoryStashAdapter::update(const number block, const bytes &data)
	{
		checkOverflow(block);

		stash[block] = data;
	}

	void InMemoryStashAdapter::get(const number block, bytes &response) const
	{
		const auto found = stash.find(block);
		if (found != stash.end())
		{
			response.insert(response.begin(), (*found).second.begin(), (*found).second.end());
		}
	}

	void InMemoryStashAdapter::remove(const number block)
	{
		stash.erase(block);
	}

	void InMemoryStashAdapter::checkOverflow(const number block) const
	{
#if INPUT_CHECKS
		if (stash.size() == capacity && stash.count(block) == 0)
		{
			throw Exception(boost::format("trying to insert over capacity (capacity %1%)") % capacity);
		}
#endif
	}

	bool InMemoryStashAdapter::exists(const number block) const
	{
		return stash.count(block) > 0;
	}

	number InMemoryStashAdapter::currentSize()
	{
		return stash.size();
	}

	void InMemoryStashAdapter::storeToFile(const string filename) const
	{
		const auto flags = fstream::out | fstream::binary | fstream::trunc;
		fstream file;

		file.open(filename, flags);
		if (!file)
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		if (stash.size() > 0)
		{
			auto blockSize	= stash.begin()->second.size();
			auto recordSize = sizeof(number) + blockSize;
			unsigned char buffer[stash.size() * recordSize];

			auto i = 0;
			for (auto &&record : stash)
			{
				number numberBuffer[1] = {record.first};
				copy((unsigned char *)numberBuffer, (unsigned char *)numberBuffer + sizeof(number), buffer + recordSize * i);
				copy(record.second.begin(), record.second.begin() + record.second.size(), buffer + recordSize * i + sizeof(number));
				i++;
			}

			file.seekg(0, file.beg);
			file.write((const char *)buffer, stash.size() * recordSize);
			file.close();
		}
	}

	void InMemoryStashAdapter::loadFromFile(const string filename, const int blockSize)
	{
		const auto flags = fstream::in | fstream::binary | fstream::ate;
		fstream file;

		file.open(filename, flags);
		if (!file)
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}
		const auto size = (int)file.tellg();
		file.seekg(0, file.beg);

		if (size > 0)
		{
			unsigned char buffer[size];
			file.read((char *)buffer, size);
			file.close();

			auto recordSize = sizeof(number) + blockSize;
			for (int i = 0; i < size; i += recordSize)
			{
				unsigned char numberBuffer[sizeof(number)];
				copy(buffer + i, buffer + i + sizeof(number), numberBuffer);
				number block = ((number *)numberBuffer)[0];

				bytes data;
				data.resize(blockSize);
				copy(buffer + i + sizeof(number), buffer + i + sizeof(number) + blockSize, data.begin());

				stash.insert({block, data});
			}
		}
	}
}
