#ifndef UTILITY_INCLUDED
#define UTILITY_INCLUDED

#include "definitions.h"

#include <string>

namespace PathORAM
{
	using namespace std;

	bytes getRandomBlock(ulong blockSize);
	ulong getRandomULong(ulong max);
	bytes encrypt(bytes key, bytes iv, bytes input, EncryptionMode mode);

	bytes fromText(string text, ulong BLOCK_SIZE);
	string toText(bytes data, ulong BLOCK_SIZE);
}

#endif
