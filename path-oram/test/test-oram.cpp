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
		MockStorage(number capacity, number userBlockSize, bytes key, number Z, number batchLimit) :
			AbsStorageAdapter(capacity, userBlockSize, key, Z, batchLimit)
		{
			_real = make_unique<InMemoryStorageAdapter>(capacity, userBlockSize, key, Z, batchLimit);

			// by default, all calls are delegated to the real object
			ON_CALL(*this, getInternal).WillByDefault([this](const vector<number> &locations, vector<bytes> &response) {
				return ((AbsStorageAdapter *)_real.get())->getInternal(locations, response);
			});
			ON_CALL(*this, setInternal).WillByDefault([this](const vector<block> &requests) {
				return ((AbsStorageAdapter *)_real.get())->setInternal(requests);
			});
		}

		// these four do not need to be mocked, they just have to exist to make class concrete
		virtual void getInternal(const number location, bytes &response) const override
		{
			return _real->getInternal(location, response);
		}

		virtual void setInternal(const number location, const bytes &raw) override
		{
			_real->setInternal(location, raw);
		}

		virtual bool supportsBatchGet() const override
		{
			return false;
		}

		virtual bool supportsBatchSet() const override
		{
			return false;
		}

		// these two need to be mocked since we want to track how and when they are called (hence ON_CALL above)
		MOCK_METHOD(void, getInternal, (const vector<number> &locations, vector<bytes> &response), (const, override));
		MOCK_METHOD(void, setInternal, ((const vector<block>)&requests), (override));

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
				for (auto j = 0uLL; j < Z; j++)
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

		for (auto &&test : tests)
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

		for (auto &&test : tests)
		{
			EXPECT_EQ(get<3>(test), oram->canInclude(get<0>(test), get<1>(test), get<2>(test)));
		}
	}

	TEST_F(ORAMTest, ReadPath)
	{
		populateStorage();

		vector<block> stashDump;
		stash->getAll(stashDump);
		EXPECT_EQ(0, stashDump.size());

		unordered_set<number> path;
		oram->readPath(10uLL, path, true);

		stashDump.clear();
		stash->getAll(stashDump);
		EXPECT_EQ(LOG_CAPACITY * Z, stashDump.size());

		vector<int> expected = {1, 3, 6, 13, 26};

		for (auto &&block : expected)
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
		bytes got;
		oram->get(CAPACITY - 1, got);
	}

	TEST_F(ORAMTest, PutNoException)
	{
		auto toPut = fromText("hello", BLOCK_SIZE);
		oram->put(CAPACITY - 1, toPut);
	}

	TEST_F(ORAMTest, GetPutSame)
	{
		auto toPut = fromText("hello", BLOCK_SIZE);
		oram->put(CAPACITY - 1, toPut);

		bytes returned;
		oram->get(CAPACITY - 1, returned);

		ASSERT_EQ("hello", toText(returned, BLOCK_SIZE));
	}

	TEST_F(ORAMTest, PutMany)
	{
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			auto toPut = fromText(to_string(id), BLOCK_SIZE);
			oram->put(id, toPut);
		}

		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			auto found = false;
			for (number location = 0; location < CAPACITY; location++)
			{
				bucket bucket;
				storage->get(location, bucket);
				for (auto &&block : bucket)
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
			auto toPut = fromText(to_string(id), BLOCK_SIZE);
			oram->put(id, toPut);
		}

		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			bytes returned;
			oram->get(id, returned);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, MultipleTooManyRequests)
	{
		vector<block> batch;
		batch.resize(BATCH_SIZE + 1);
		ASSERT_ANY_THROW({
			vector<bytes> response;
			oram->multiple(batch, response);
		});
	}

	TEST_F(ORAMTest, MultipleCheckCache)
	{
		using ::testing::An;
		using ::testing::NiceMock;
		using ::testing::Truly;

		auto storage = make_shared<NiceMock<MockStorage>>(CAPACITY + Z, BLOCK_SIZE, bytes(), Z, 0);

		// make sure main storage is not called
		auto oram = make_unique<ORAM>(LOG_CAPACITY, BLOCK_SIZE, Z, storage, make_unique<InMemoryPositionMapAdapter>(CAPACITY * Z + Z), stash, true, BATCH_SIZE);

		vector<block> batch;
		for (number id = 0; id < BATCH_SIZE; id++)
		{
			batch.push_back({id, bytes()});
		}

		const auto noDupsPredicate = [](const vector<number> &locations) -> bool {
			auto tmp = vector<number>(locations.begin(), locations.end());

			sort(tmp.begin(), tmp.end());
			auto it = unique(tmp.begin(), tmp.end());
			return it == tmp.end();
		};

		EXPECT_CALL(*storage, getInternal(Truly(noDupsPredicate), An<vector<bytes> &>())).Times(1);
		EXPECT_CALL(*storage, setInternal(An<const vector<block> &>())).Times(1);

		vector<bytes> response;
		oram->multiple(batch, response);
	}

	TEST_F(ORAMTest, MultipleGet)
	{
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			auto toPut = fromText(to_string(id), BLOCK_SIZE);
			oram->put(id, toPut);
		}

		vector<block> batch;
		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			batch.push_back({id, bytes()});
			if (id % BATCH_SIZE == 0 || id == CAPACITY * Z - 6)
			{
				if (batch.size() > 0)
				{
					vector<bytes> response;
					oram->multiple(batch, response);
					ASSERT_EQ(batch.size(), response.size());

					for (number i = 0; i < batch.size(); i++)
					{
						EXPECT_EQ(to_string(batch[i].first), toText(response[i], BLOCK_SIZE));
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
					vector<bytes> response;
					oram->multiple(batch, response);

					ASSERT_EQ(batch.size(), response.size());
					for (number i = 0; i < batch.size(); i++)
					{
						EXPECT_EQ(batch[i].second, response[i]);
					}
				}

				batch.clear();
			}
		}

		for (number id = 0; id < CAPACITY * Z - 5; id++)
		{
			bytes returned;
			oram->get(id, returned);
			EXPECT_EQ(to_string(id), toText(returned, BLOCK_SIZE));
		}
	}

	TEST_F(ORAMTest, BulkLoad)
	{
		vector<block> batch;
		for (number id = 0; id < 3 * CAPACITY * Z / 4 + 1; id++)
		{
			batch.push_back({id, fromText(to_string(id), BLOCK_SIZE)});
		}

		oram->load(batch);

		for (number id = 0; id < 3 * CAPACITY * Z / 4 + 1; id++)
		{
			bytes returned;
			oram->get(id, returned);
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
		gets.reserve(CAPACITY * Z);

		for (number id = 0; id < CAPACITY * Z; id++)
		{
			auto toPut = fromText(to_string(id), BLOCK_SIZE);
			oram->put(id, toPut);
			vector<block> stashDump;
			stash->getAll(stashDump);
			puts.push_back(stashDump.size());
		}

		for (number id = 0; id < CAPACITY * Z; id++)
		{
			bytes returned;
			oram->get(id, returned);
			vector<block> stashDump;
			stash->getAll(stashDump);
			gets.push_back(stashDump.size());
		}

		EXPECT_GE(LOG_CAPACITY * Z * 2, *max_element(gets.begin(), gets.end()));
		EXPECT_EQ(0, *min_element(puts.begin(), puts.end()));
	}

	TEST_F(ORAMTest, LeavesForLocation)
	{
		const auto HEIGHT = 5;

		auto smallOram = make_unique<ORAM>(HEIGHT, BLOCK_SIZE, Z);
		for (auto location = 1; location < (1 << HEIGHT); location++)
		{
			const auto [left, right] = smallOram->leavesForLocation(location);
			const auto level		 = (number)floor(log2(location));

			for (auto leaf = left; leaf <= right; leaf++)
			{
				EXPECT_EQ(location, smallOram->bucketForLevelLeaf(level, leaf));
				for (auto anotherLeaf = left; anotherLeaf <= right; anotherLeaf++)
				{
					EXPECT_TRUE(smallOram->canInclude(leaf, anotherLeaf, level));
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
