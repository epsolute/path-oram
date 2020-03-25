#include "utility.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <iomanip>
#include <openssl/rand.h>
#include <sstream>
#include <vector>

namespace PathORAM
{
	using namespace std;

	bytes getRandomBlock(ulong blockSize)
	{
		uchar material[blockSize];
#ifdef TESTING
		for (int i = 0; i < blockSize; i++)
		{
			material[i] = (uchar)rand();
		}
#else
		RAND_bytes(material, blockSize);
#endif
		return bytes(material, material + blockSize);
	}

	// TODO not secure!
	ulong getRandomULong(ulong max)
	{
		ulong material[1];
#ifdef TESTING
		auto intMaterial = (int *)material;
		intMaterial[0]   = rand();
		intMaterial[1]   = rand();
#else
		RAND_bytes((uchar *)material, sizeof(ulong));
#endif
		return material[0] % max;
	}

	void seedRandom(int seed)
	{
		srand(seed);
	}

	bytes fromText(string text, ulong BLOCK_SIZE)
	{
		stringstream padded;
		padded << setw(BLOCK_SIZE - 1) << left << text << endl;
		text = padded.str();

		return bytes((uchar *)text.c_str(), (uchar *)text.c_str() + text.length());
	}

	string toText(bytes data, ulong BLOCK_SIZE)
	{
		char buffer[BLOCK_SIZE];
		memset(buffer, 0, sizeof buffer);
		copy(data.begin(), data.end(), buffer);
		buffer[BLOCK_SIZE - 1] = '\0';
		auto text			   = string(buffer);
		boost::algorithm::trim_right(text);
		return text;
	}
}
