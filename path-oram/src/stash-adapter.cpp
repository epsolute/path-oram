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
		this->stash.reserve(this->capacity);
	}

	unordered_map<number, bytes> InMemoryStashAdapter::getAll()
	{
		auto result = this->stash;
		return result;
	}

	void InMemoryStashAdapter::add(number block, bytes data)
	{
		this->checkOverflow(block);

		this->stash.insert({block, data});
	}

	void InMemoryStashAdapter::update(number block, bytes data)
	{
		this->checkOverflow(block);

		this->stash[block] = data;
	}

	bytes InMemoryStashAdapter::get(number block)
	{
		return this->stash[block];
	}

	void InMemoryStashAdapter::remove(number block)
	{
		this->stash.erase(block);
	}

	void InMemoryStashAdapter::checkOverflow(number block)
	{
		if (this->stash.size() == this->capacity && this->stash.count(block) == 0)
		{
			throw boost::str(boost::format("trying to insert over capacity (capacity %1%)") % this->capacity);
		}
	}

	bool InMemoryStashAdapter::exists(number block)
	{
		return this->stash.count(block);
	}
}
