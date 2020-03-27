#pragma once

#include "definitions.h"

#include <string>

namespace PathORAM
{
	using namespace std;

	bytes getRandomBlock(number blockSize);
	number getRandomULong(number max);
	bytes encrypt(bytes key, bytes iv, bytes input, EncryptionMode mode);

	bytes fromText(string text, number BLOCK_SIZE);
	string toText(bytes data, number BLOCK_SIZE);
}
