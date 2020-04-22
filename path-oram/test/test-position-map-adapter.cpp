#include "definitions.h"
#include "position-map-adapter.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <algorithm>
#include <boost/format.hpp>
#include <math.h>
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
		inline static const number CAPACITY = 10;

		inline static const number Z		  = 3;
		inline static const number BLOCK_SIZE = 2 * AES_BLOCK_SIZE;

		protected:
		unique_ptr<AbsPositionMapAdapter> adapter;

		PositionMapAdapterTest()
		{
			auto logCapacity = max((number)ceil(log(CAPACITY * Z) / log(2)), 3uLL);
			auto capacity	 = (1 << logCapacity) * Z;

			auto type = GetParam();
			switch (type)
			{
				case PositionMapAdapterTypeInMemory:
					this->adapter = make_unique<InMemoryPositionMapAdapter>(CAPACITY);
					break;
				case PositionMapAdapterTypeORAM:
					this->adapter = make_unique<ORAMPositionMapAdapter>(
						make_unique<ORAM>(
							logCapacity,
							BLOCK_SIZE,
							Z,
							make_unique<InMemoryStorageAdapter>(capacity * Z + Z, BLOCK_SIZE, bytes(), Z),
							make_unique<InMemoryPositionMapAdapter>(capacity * Z + Z),
							make_unique<InMemoryStashAdapter>(3 * logCapacity * Z)));
					break;
				default:
					throw Exception(boost::format("TestingPositionMapAdapterType %2% is not implemented") % type);
			}
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

	TEST_P(PositionMapAdapterTest, LoadStore)
	{
		if (GetParam() == PositionMapAdapterTypeInMemory)
		{
			const auto filename = "position-map.bin";
			const auto expected = 56uLL;

			auto map = make_unique<InMemoryPositionMapAdapter>(CAPACITY);
			map->set(CAPACITY - 1, expected);
			map->storeToFile(filename);
			map.reset();

			map = make_unique<InMemoryPositionMapAdapter>(CAPACITY);
			map->loadFromFile(filename);
			auto read = map->get(CAPACITY - 1);
			EXPECT_EQ(expected, read);

			remove(filename);
		}
		else
		{
			SUCCEED();
		}
	}

	TEST_P(PositionMapAdapterTest, LoadStoreFileError)
	{
		if (GetParam() == PositionMapAdapterTypeInMemory)
		{
			auto map = make_unique<InMemoryPositionMapAdapter>(CAPACITY);
			ASSERT_ANY_THROW(map->storeToFile("/error/path/should/not/exist"));
			ASSERT_ANY_THROW(map->loadFromFile("/error/path/should/not/exist"));
		}
		else
		{
			SUCCEED();
		}
	}

	TEST_P(PositionMapAdapterTest, BlockOutOfBounds)
	{
		ASSERT_ANY_THROW(adapter->get(CAPACITY * 100));
		ASSERT_ANY_THROW(adapter->set(CAPACITY * 100, 56uLL));
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
				throw Exception(boost::format("TestingPositionMapAdapterType %2% is not implemented") % input.param);
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
