#include "oram.hpp"
#include "utility.hpp"

#include <iostream>

using namespace std;
using namespace PathORAM;

int main()
{
	cout << "Running random small simulation using compiled shared library..." << endl;

	const auto LOG_CAPACITY = 5;
	const auto BLOCK_SIZE   = 32;
	const auto Z			= 3;
	const auto CAPACITY		= (1 << LOG_CAPACITY) * Z;
	const auto ELEMENTS		= (CAPACITY / 4) * 3;

	cout << "LOG_CAPACITY: " << LOG_CAPACITY << endl;
	cout << "BLOCK_SIZE: " << BLOCK_SIZE << endl;
	cout << "Z: " << Z << endl;
	cout << "CAPACITY: " << CAPACITY << endl;
	cout << "ELEMENTS: " << ELEMENTS << endl;

	auto oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z);

	// put all
	for (ulong id = 0; id < ELEMENTS; id++)
	{
		auto data = fromText(to_string(id), BLOCK_SIZE);
		oram->put(id, data);
	}

	// get all
	for (ulong id = 0; id < ELEMENTS; id++)
	{
		oram->get(id);
	}

	// random operations
	for (ulong i = 0; i < 2 * ELEMENTS; i++)
	{
		auto id   = getRandomULong(ELEMENTS);
		auto read = getRandomULong(2) == 0;
		if (read)
		{
			// get
			oram->get(id);
		}
		else
		{
			auto data = fromText(to_string(ELEMENTS + getRandomULong(ELEMENTS)), BLOCK_SIZE);
			oram->put(id, data);
		}
	}

	delete oram;

	cout << "Successful!" << endl;

	return 0;
}
