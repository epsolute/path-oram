#include "stash-adapter.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

	AbsStashAdapter::~AbsStashAdapter(){};

	InMemoryStashAdapter::~InMemoryStashAdapter() {}

	InMemoryStashAdapter::InMemoryStashAdapter(ulong capacity) :
		capacity(capacity)
	{
		this->stash = unordered_map<ulong, bytes>();
		this->stash.reserve(this->capacity);
	}

	unordered_map<ulong, bytes> InMemoryStashAdapter::getAll()
	{
		auto result = this->stash;
		return result;
	}

	void InMemoryStashAdapter::add(ulong block, bytes data)
	{
		this->checkOverflow(block);

		this->stash.insert({block, data});
	}

	void InMemoryStashAdapter::update(ulong block, bytes data)
	{
		this->checkOverflow(block);

		this->stash[block] = data;
	}

	bytes InMemoryStashAdapter::get(ulong block)
	{
		return this->stash[block];
	}

	void InMemoryStashAdapter::remove(ulong block)
	{
		this->stash.erase(block);
	}

	void InMemoryStashAdapter::checkOverflow(ulong block)
	{
		if (this->stash.size() == this->capacity && this->stash.count(block) == 0)
		{
			throw boost::format("trying to insert over capacity (capacity %2%)") % this->capacity;
		}
	}
}
