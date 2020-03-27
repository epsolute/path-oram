#include "stash-adapter.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsStashAdapter::~AbsStashAdapter(){};

	InMemoryStashAdapter::~InMemoryStashAdapter() {}

	InMemoryStashAdapter::InMemoryStashAdapter(number capacity) :
		capacity(capacity)
	{
		this->stash = unordered_map<number, bytes>();
		stash.reserve(capacity);
	}

	unordered_map<number, bytes> InMemoryStashAdapter::getAll()
	{
		auto result = stash;
		return result;
	}

	void InMemoryStashAdapter::add(number block, bytes data)
	{
		checkOverflow(block);

		stash.insert({block, data});
	}

	void InMemoryStashAdapter::update(number block, bytes data)
	{
		checkOverflow(block);

		stash[block] = data;
	}

	bytes InMemoryStashAdapter::get(number block)
	{
		return stash[block];
	}

	void InMemoryStashAdapter::remove(number block)
	{
		stash.erase(block);
	}

	void InMemoryStashAdapter::checkOverflow(number block)
	{
		if (stash.size() == capacity && stash.count(block) == 0)
		{
			throw boost::str(boost::format("trying to insert over capacity (capacity %1%)") % capacity);
		}
	}

	bool InMemoryStashAdapter::exists(number block)
	{
		return stash.count(block);
	}
}
