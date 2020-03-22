#include "definitions.h"
#include "oram.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace PathORAM;

class ORAMTest : public ::testing::Test
{
	public:
	inline static const ulong LOG_CAPACITY = 10;
	inline static const ulong Z			   = 3;
	inline static const ulong BLOCK_SIZE   = 256;

	protected:
	ORAM* oram;
	AbsStorageAdapter* storage = new InMemoryStorageAdapter((1 << LOG_CAPACITY) + Z, BLOCK_SIZE);
	AbsPositionMapAdapter* map = new InMemoryPositionMapAdapter((1 << LOG_CAPACITY) + Z);

	ORAMTest()
	{
		this->oram = new ORAM(LOG_CAPACITY, BLOCK_SIZE, Z, this->storage, this->map);
	}

	~ORAMTest() override
	{
		delete oram;
		delete storage;
		delete map;
	}
};

TEST_F(ORAMTest, Initialization)
{
	SUCCEED();
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
