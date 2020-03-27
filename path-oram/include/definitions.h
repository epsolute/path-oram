#pragma once

#include <climits>
#include <vector>

#define KEYSIZE 32

#define TEST_SEED 0x13

namespace PathORAM
{
	using number = unsigned long long;
	using uchar  = unsigned char;
	using bytes  = std::vector<uchar>;

	enum EncryptionMode
	{
		ENCRYPT,
		DECRYPT
	};
}
