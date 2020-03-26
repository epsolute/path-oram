#include "definitions.h"
#include "position-map-adapter.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <algorithm>
#include <boost/format.hpp>
#include <openssl/aes.h>

using namespace std;

namespace PathORAM
{
	enum TestingPositionMapAdapterType
	{
		PositionMapAdapterTypeInMemory,
		PositionMapAdapterTypeORAM
	};

	class PositionMapAdapterTest : public testing::TestWithParam<TestingPositionMapAdapterType>
	{
		public:
		inline static const ulong CAPACITY = 10;

		inline static const ulong Z			 = 3;
		inline static const ulong BLOCK_SIZE = 2 * AES_BLOCK_SIZE;

		protected:
		AbsPositionMapAdapter* adapter;

		ORAM* oram;
		AbsStorageAdapter* storage;
		AbsStashAdapter* stash;
		AbsPositionMapAdapter* map;

		PositionMapAdapterTest()
		{
			auto logCapacity = min((ulong)ceil(log(CAPACITY) / log(2)), 3uLL);
			auto capacity	= (1 << logCapacity) * Z;

			this->storage = new InMemoryStorageAdapter(capacity + Z, BLOCK_SIZE, bytes());
			this->map	 = new InMemoryPositionMapAdapter(capacity + Z);
			this->stash   = new InMemoryStashAdapter(3 * logCapacity * Z);

			this->oram = new ORAM(
				logCapacity,
				BLOCK_SIZE,
				Z,
				this->storage,
				this->map,
				this->stash);

			auto type = GetParam();
			switch (type)
			{
				case PositionMapAdapterTypeInMemory:
					this->adapter = new InMemoryPositionMapAdapter(CAPACITY);
					break;
				case PositionMapAdapterTypeORAM:
					this->adapter = new ORAMPositionMapAdapter(this->oram);
					break;
				default:
					throw boost::format("TestingPositionMapAdapterType %2% is not implemented") % type;
			}
		}

		~PositionMapAdapterTest() override
		{
			delete adapter;

			delete oram;
			delete map;
			delete storage;
			delete stash;
		}
	};

	TEST_P(PositionMapAdapterTest, Initialization)
	{
		SUCCEED();
	}

	TEST_P(PositionMapAdapterTest, ReadWriteNoCrash)
	{
		EXPECT_NO_THROW({
			adapter->set(CAPACITY - 1, 56uLL);
			adapter->get(CAPACITY - 2);
		});
	}

	TEST_P(PositionMapAdapterTest, BlockOutOfBounds)
	{
		ASSERT_ANY_THROW(adapter->get(CAPACITY * 10));
		ASSERT_ANY_THROW(adapter->set(CAPACITY * 10, 56uLL));
	}

	TEST_P(PositionMapAdapterTest, ReadWhatWasWritten)
	{
		auto leaf = 56uLL;

		adapter->set(CAPACITY - 1, leaf);
		auto returned = adapter->get(CAPACITY - 1);

		ASSERT_EQ(leaf, returned);
	}

	TEST_P(PositionMapAdapterTest, Override)
	{
		auto old = 56uLL, _new = 25uLL;

		adapter->set(CAPACITY - 1, old);
		adapter->set(CAPACITY - 1, _new);

		auto returned = adapter->get(CAPACITY - 1);

		ASSERT_EQ(_new, returned);
	}

	string printTestName(testing::TestParamInfo<TestingPositionMapAdapterType> input)
	{
		switch (input.param)
		{
			case PositionMapAdapterTypeInMemory:
				return "InMemory";
			case PositionMapAdapterTypeORAM:
				return "ORAM";
			default:
				throw boost::format("TestingPositionMapAdapterType %2% is not implemented") % input.param;
		}
	}

	INSTANTIATE_TEST_SUITE_P(PositionMapSuite, PositionMapAdapterTest, testing::Values(PositionMapAdapterTypeInMemory, PositionMapAdapterTypeORAM), printTestName);
}

int main(int argc, char** argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
