#pragma once

#include "definitions.h"
#include "oram.hpp"

#include <iostream>

namespace PathORAM
{
	using namespace std;

	/**
	 * @brief Abstraction over position map
	 */
	class AbsPositionMapAdapter
	{
		public:
		/**
		 * @brief get mapped leaf for block
		 *
		 * @param block block in question
		 * @return number leaf mapped to this block
		 */
		virtual number get(const number block) const = 0;

		/**
		 * @brief map a leaf to the block
		 *
		 * @param block block in question
		 * @param leaf leaf in question
		 */
		virtual void set(const number block, const number leaf) = 0;

		virtual ~AbsPositionMapAdapter() = 0;
	};

	/**
	 * @brief In-memory implementation of position adapter.
	 *
	 * Uses simple array of numbers as an underlying storage.
	 */
	class InMemoryPositionMapAdapter : public AbsPositionMapAdapter
	{
		private:
		number* const map;
		const number capacity; // maximum capacity, array size

		/**
		 * @brief helper that throws exception if out-of-bounds access occurs
		 *
		 * @param block accessed block
		 */
		void checkCapacity(const number block) const;

		public:
		/**
		 * @brief Construct a new In Memory Position Map Adapter object
		 *
		 * @param capacity maximum capacity (defines the allocated array size)
		 */
		InMemoryPositionMapAdapter(const number capacity);

		~InMemoryPositionMapAdapter() final;
		number get(const number block) const final;
		void set(const number block, const number leaf) final;

		/**
		 * @brief write state to a binary file
		 *
		 * @param filename the name of the file to write to
		 */
		void storeToFile(const string filename) const;

		/**
		 * @brief read state from a binary file
		 *
		 * @param filename the name of the file to read from
		 */
		void loadFromFile(const string filename);
	};

	class ORAM;

	/**
	 * @brief A PathORAM implementation of the position map adapter.
	 *
	 * Uses an instance of PathORAM as the sotrage for the map.
	 */
	class ORAMPositionMapAdapter : public AbsPositionMapAdapter
	{
		private:
		const shared_ptr<ORAM> oram;

		friend class ORAMBigTest;

		public:
		/**
		 * @brief Construct a new ORAMPositionMapAdapter object
		 *
		 * @param oram the intialized (with proper capacities) ORAM that will be used as a position map storage
		 */
		ORAMPositionMapAdapter(const shared_ptr<ORAM> oram);
		~ORAMPositionMapAdapter() final;
		number get(const number block) const final;
		void set(const number block, const number leaf) final;
	};
}
