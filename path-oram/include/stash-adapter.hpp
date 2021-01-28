#pragma once

#include "definitions.h"

#include <iostream>
#include <unordered_map>

namespace PathORAM
{
	using namespace std;

	/**
	 * @brief Abstraction over stash
	 *
	 */
	class AbsStashAdapter
	{
		public:
		/**
		 * @brief get the all blocks (ID and data) from the stash in a form of a pseudorandomly permuted vector.
		 *
		 * @pararm response the pseudorandomly permuted vector of objects { ID, data }
		 */
		virtual void getAll(vector<block> &response) const = 0;

		/**
		 * @brief put an object in the stash
		 *
		 * Will NOT override object with the same key.
		 *
		 * @param block ID of the block
		 * @param data data part of the object
		 */
		virtual void add(const number block, const bytes &data) = 0;

		/**
		 * @brief change an object in the stash (by ID)
		 *
		 * Will override object with the same key.
		 * If none exists, will add it.
		 *
		 * @param block ID of the block
		 * @param data data part of the object
		 */
		virtual void update(const number block, const bytes &data) = 0;

		/**
		 * @brief retrieve the object by ID
		 *
		 * @param block ID of the block
		 * @param response data part of the object
		 */
		virtual void get(const number block, bytes &response) const = 0;

		/**
		 * @brief removes the object by ID
		 *
		 * Does nothing if object does not exist.
		 *
		 * @param block block ID of the block
		 */
		virtual void remove(const number block) = 0;

		virtual ~AbsStashAdapter() = 0;

		protected:
		/**
		 * @brief (for testing) check if an object exists in the stash
		 *
		 * @param block block ID of the block
		 * @return true if an object with such ID exists in the stash
		 * @return false otherwise
		 */
		virtual bool exists(const number block) const = 0;

		friend class ORAMTest_ReadPath_Test;
		friend class ORAMTest_PutMany_Test;
	};

	/**
	 * @brief In-memory implementation of the stash adapter
	 *
	 * Uses STD unordered map as the underlying storage
	 *
	 */
	class InMemoryStashAdapter : public AbsStashAdapter
	{
		private:
		unordered_map<number, bytes> stash;
		const number capacity;

		/**
		 * @brief thorows exception if an insertion of this block will cause an overflow (stash size growing beyond capacity)
		 *
		 * @param block block ID in question
		 */
		void checkOverflow(const number block) const;

		bool exists(const number block) const final;

		friend class ORAMTest_ReadPath_Test;

		public:
		/**
		 * @brief Construct a new In Memory Stash Adapter object
		 *
		 * @param capacity the maximum number of objects that should be allowed in the stash
		 */
		InMemoryStashAdapter(number capacity);

		~InMemoryStashAdapter() final;

		void getAll(vector<block> &response) const final;
		void add(const number block, const bytes &data) final;
		void update(const number block, const bytes &data) final;
		void get(const number block, bytes &response) const final;
		void remove(const number block) final;

		/**
		 * @brief Returns the current size of the stash (in blocks)
		 *
		 * @return number the current size of the stash (in blocks)
		 */
		number currentSize();

		/**
		 * @brief write state to a binary file
		 *
		 * @param filename the name of the file to write to
		 */
		void storeToFile(const string filename) const;

		/**
		 * @brief read state from a binary file
		 *
		 * @param filename filename the name of the file to read from
		 * @param blockSize the size of the block in the stash (same as when storeToFile was used)
		 */
		void loadFromFile(const string filename, const int blockSize);
	};
}
