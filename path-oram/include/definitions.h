#ifndef DEFINITIONS_INCLUDED
#define DEFINITIONS_INCLUDED

#include <vector>

#define KEYSIZE 32

namespace PathORAM
{
	using ulong = unsigned long long;
	using uchar = unsigned char;
	using bytes = std::vector<uchar>;

	enum EncryptionMode
	{
		ENCRYPT,
		DECRYPT
	};
}

#endif
