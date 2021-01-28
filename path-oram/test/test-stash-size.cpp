#include "oram.hpp"
#include "utility.hpp"

#include <iostream>

using namespace std;
using namespace PathORAM;

int main()
{
	cout << "Running simulations to observe stash usage..." << endl;

	PathORAM::__blockCipherMode = PathORAM::BlockCipherMode::NONE;

	const auto LOG_CAPACITY = 11;
	const auto BLOCK_SIZE	= 32;
	const auto Z			= 3;
	const auto CAPACITY		= (1 << LOG_CAPACITY);
	const auto ELEMENTS		= 100000 / 64;
	const auto RUNS			= 100000;

	cout << "LOG_CAPACITY: " << LOG_CAPACITY << endl;
	cout << "BLOCK_SIZE: " << BLOCK_SIZE << endl;
	cout << "Z: " << Z << endl;
	cout << "CAPACITY: " << CAPACITY << endl;
	cout << "ELEMENTS: " << ELEMENTS << endl;

	vector<pair<number, bytes>> data;
	for (number id = 0; id < ELEMENTS; id++)
	{
		data.push_back({id, PathORAM::fromText(to_string(id), BLOCK_SIZE)});
	}

	auto stash = make_shared<InMemoryStashAdapter>(3 * LOG_CAPACITY * Z);

	auto oram = make_unique<ORAM>(
		LOG_CAPACITY,
		BLOCK_SIZE,
		Z,
		make_shared<InMemoryStorageAdapter>((1 << LOG_CAPACITY), BLOCK_SIZE, bytes(), Z),
		make_shared<InMemoryPositionMapAdapter>(((1 << LOG_CAPACITY) * Z) + Z),
		stash);

	oram->load(data);

	number largest = 0;
	for (number run = 0; run < RUNS; run++)
	{
		auto id = getRandomULong(ELEMENTS);
		bytes returned;
		oram->get(id, returned);

		auto size = stash->currentSize();
		if (size > largest) {
			cout << size << endl;
			largest = size;
		}
	}

	cout << "Successful!" << endl;

	return 0;
}
