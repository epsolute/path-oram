#include "utility.hpp"

#include <openssl/rand.h>
#include <vector>

namespace PathORAM
{
	using namespace std;

	bytes getRandomBlock(ulong blockSize)
	{
		unsigned char material[blockSize];
		RAND_bytes(material, blockSize);
		return bytes(material, material + blockSize);
	}

	// TODO not secure!
	ulong getRandomULong(ulong max)
	{
		ulong material[1];
		RAND_bytes((unsigned char *)material, 1);
		return material[0] % max;
	}
}
