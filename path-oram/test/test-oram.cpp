#include "definitions.h"
#include "oram.hpp"
#include "utility.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace std;

namespace PathORAM
{
	class MockStorage : public AbsStorageAdapter
	{
		public:
		MockStorage(number capacity, number userBlockSize, bytes key, number Z) :
			AbsStorageAdapter(capacity, userBlockSize, key, Z)
		{
			_real = make_unique<InMemoryStorageAdapter>(capacity, userBlockSize, key, Z);

			// by default, all calls are delegated to the real object
			ON_CALL(*this, getInternal).WillByDefault([this](vector<number> locations) {
				return ((AbsStorageAdapter*)_real.get())->getInternal(locations);
			});
			ON_CALL(*this, setInternal).WillByDefault([this](vector<block> requests) {
				return ((AbsStorageAdapter*)_real.get())->setInternal(requests);
			});
		}

		// these two do not need to be mocked, they just have to exist to make class concrete
		virtual bytes getInternal(number location) override
		{
			return _real->getInternal(location);
		}

		virtual void setInternal(number location, bytes raw) override
		{
			_real->setInternal(location, raw);
		}

		// these two need to be mocked since we want tot rack how and when they are called (hence ON_CALL above)
		MOCK_METHOD(vector<bytes>, getInternal, (vector<number> locations), (override));
		MOCK_METHOD(void, setInternal, ((vector<block>)requests), (override));

		private:
		unique_ptr<InMemoryStorageAdapter> _real;
	};

	class ORAMTest : public ::testing::Test
	{
		public:
		inline static const number LOG_CAPACITY = 5;
		inline static const number Z			= 3;
		inline static const number BLOCK_SIZE	= 32;
		inline static const number BATCH_SIZE	= 10;

		inline static const number CAPACITY = (1 << LOG_CAPACITY);

		protected:
		unique_ptr<ORAM> oram;
		shared_ptr<AbsStorageAdapter> storage = make_shared<InMemoryStorageAdapter>(CAPACITY + Z, BLOCK_SIZE, bytes(), Z);
		shared_ptr<AbsStashAdapter> stash	  = make_shared<InMemoryStashAdapter>(3 * LOG_CAPACITY * Z);

		ORAMTest()
		{
			this->oram = make_unique<ORAM>(
				LOG_CAPACITY,
				BLOCK_SIZE,
				Z,
				storage,
				make_unique<InMemoryPositionMapAdapter>(CAPACITY * Z + Z),
				stash,
				true,
				BATCH_SIZE);
		}

		void populateStorage()
		{
			for (number i = 0; i < CAPACITY; i++)
			{
				bucket bucket;
				for (auto j = 0; j < Z; j++)
				{
					bucket.push_back({i * Z + j, bytes()});
				}
				storage->set(i, bucket);
			}
		}
	};

	TEST_F(ORAMTest, Helpers)
	{
		ASSERT_EQ(toText(fromText("hello", BLOCK_SIZE), BLOCK_SIZE), "hello");
		ASSERT_EQ(BLOCK_SIZE, fromText("hello", BLOCK_SIZE).size());
	}

	TEST_F(ORAMTest, Initialization)
	{
		SUCCEED();
	}

	TEST_F(ORAMTest, InitializationShorthand)
	{
		ASSERT_NO_THROW(auto oram = make_unique<ORAM>(LOG_CAPACITY, BLOCK_SIZE, Z));
	}

	TEST_F(ORAMTest, BucketFromLevelLeaf)
	{
		vector<pair<number, vector<number>>> tests =
			{
				{6, {1, 2, 5, 11, 22}},
				{8, {1, 3, 6, 12, 24}},
				{14, {1, 3, 7, 15, 30}},
			};

		for (auto test : tests)
		{
			for (number level = 0; level < LOG_CAPACITY; level++)
			{
				EXPECT_EQ(test.second[level], oram->bucketForLevelLeaf(level, test.first));
			}
		}
	}

	TEST_F(ORAMTest, CanInclude)
	{
		vector<tuple<number, number, number, bool>> tests =
			{
				{8, 11, 2, true},
				{8, 11, 3, false},
				{0, 11, 0, true},
				{0, 11, 1, false},
				{0, 11, 2, false},
			};

		for (auto test : tests)
		{
			EXPECT_EQ(get<3>(test), oram->canInclude(get<0>(test), get<1>(test), get<2>(test)));
		}
	}

	TEST_F(ORAMTest, ReadPath)
	{
		populateStorage();

		EXPECT_EQ(0, stash->getAll().size());

		auto path = oram->readPath(10uLL);

		EXPECT_EQ(LOG_CAPACITY * Z, stash->getAll().size());

		vector<int> expected = {1, 3, 6, 13, 26};

		for (auto block : expected)
		{
			for (number i = 0; i < Z; i++)
			{
				auto id = block * Z + i;
				EXPECT_TRUE(stash->exists(id));
			}
		}
	}

	TEST_F(ORAMTest, GetNoException)
	{
		oram->get(CAPACITY - 1);
	}

	TEST_F(ORAMTest, PutNoException)
	{
		oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE));
	}

	TEST_F(ORAMTest, GetPutSame)
	{
		oram->put(CAPACITY - 1, fromText("hello", BLOCK_SIZE));
		auto returned = oram->get(CAPACITY - 1);

		ASSERT_EQ("hello", toText(returned, BLOCK_SIZE));
	}

	TEST_F(ORAMTest, PutMany)
	{
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			oram->put(id, fromText(to_string(id), BLOCK_SIZE));
		}

		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			auto found = false;
			for (number location = 0; location < CAPACITY; location++)
			{
				auto bucket = storage->get(location);
				for (auto block : bucket)
				{
					if (block.first == id)
					{
						found = true;
						break;
					}
				}
				if (found)
				{
					break;
				}
			}
			if (!found)
			{
				found = stash->exists(id);
			}
			EXPECT_TRUE(found);
		}
	}

	TEST_F(ORAMTest, PutGetMany)
	{
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			oram->put(id, fromText(to_string(id), BLOCK_SIZE));
		}

		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			auto returned = oram->get(id);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, MultipleTooManyRequests)
	{
		vector<block> batch;
		batch.resize(BATCH_SIZE + 1);
		ASSERT_ANY_THROW(oram->multiple(batch));
	}

	TEST_F(ORAMTest, MultipleCheckCache)
	{
		using ::testing::An;
		using ::testing::NiceMock;
		using ::testing::Truly;

		auto storage = make_shared<NiceMock<MockStorage>>(CAPACITY + Z, BLOCK_SIZE, bytes(), Z);

		// make sure main storage is not called
		auto oram = make_unique<ORAM>(LOG_CAPACITY, BLOCK_SIZE, Z, storage, make_unique<InMemoryPositionMapAdapter>(CAPACITY * Z + Z), stash, true, BATCH_SIZE);

		vector<block> batch;
		for (number id = 0; id < BATCH_SIZE; id++)
		{
			batch.push_back({id, bytes()});
		}

		auto noDupsPredicate = [](vector<number> locations) -> bool {
			sort(locations.begin(), locations.end());
			auto it = unique(locations.begin(), locations.end());
			return it == locations.end();
		};

		EXPECT_CALL(*storage, getInternal(Truly(noDupsPredicate))).Times(1);
		EXPECT_CALL(*storage, setInternal(An<vector<block>>())).Times(1);

		oram->multiple(batch);
	}

	TEST_F(ORAMTest, MultipleGet)
	{
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			oram->put(id, fromText(to_string(id), BLOCK_SIZE));
		}

		vector<block> batch;
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			batch.push_back({id, bytes()});
			if (id % BATCH_SIZE == 0 || id == CAPACITY * Z - 6)
			{
				if (batch.size() > 0)
				{
					auto returned = oram->multiple(batch);
					ASSERT_EQ(batch.size(), returned.size());

					for (number i = 0; i < batch.size(); i++)
					{
						EXPECT_EQ(to_string(batch[i].first), toText(returned[i], BLOCK_SIZE));
					}

					batch.clear();
				}
			}
		}
	}

	TEST_F(ORAMTest, MultiplePut)
	{
		vector<block> batch;
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			batch.push_back({id, fromText(to_string(id), BLOCK_SIZE)});
			if (id % BATCH_SIZE == 0 || id == CAPACITY * Z - 6)
			{
				if (batch.size() > 0)
				{
					auto returned = oram->multiple(batch);
					ASSERT_EQ(batch.size(), returned.size());
					for (number i = 0; i < batch.size(); i++)
					{
						EXPECT_EQ(batch[i].second, returned[i]);
					}
				}

				batch.clear();
			}
		}

		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			auto returned = oram->get(id);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, BulkLoad)
	{
		vector<block> batch;
		for (number id = 0; id < 3 * CAPACITY * Z / 4; id++)
		{
			batch.push_back({id, fromText(to_string(id), BLOCK_SIZE)});
		}

		oram->load(batch);

		for (number id = 0; id < 3 * CAPACITY * Z / 4; id++)
		{
			auto returned = oram->get(id);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, BulkLoadTooMany)
	{
		vector<block> batch;
		for (number id = 0; id < CAPACITY * Z + 1; id++)
		{
			batch.push_back({id, fromText(to_string(id), BLOCK_SIZE)});
		}

		ASSERT_ANY_THROW(oram->load(batch));
	}

	TEST_F(ORAMTest, StashUsage)
	{
		vector<int> puts, gets;
		puts.reserve(CAPACITY * Z);
		gets.reserve(CAPACITY * Z - 5);

		for (number id = 0; id < CAPACITY * Z; id++)
		{
			oram->put(id, fromText(to_string(id), BLOCK_SIZE));
			puts.push_back(stash->getAll().size());
		}

		for (number id = 0; id < CAPACITY * Z; id++)
		{
			oram->get(id);
			gets.push_back(stash->getAll().size());
		}

		EXPECT_GE(LOG_CAPACITY * Z * 2, *max_element(gets.begin(), gets.end()));
		EXPECT_EQ(0, *min_element(puts.begin(), puts.end()));
	}
}

int main(int argc, char** argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
