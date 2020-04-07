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
		virtual number get(number block) = 0;

		/**
		 * @brief map a leaf to the block
		 *
		 * @param block block in question
		 * @param leaf leaf in question
		 */
		virtual void set(number block, number leaf) = 0;

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
		number *map;
		number capacity; // maximum capacity, array size

		/**
		 * @brief helper that throws exception if ou-of-bounds access occurs
		 *
		 * @param block accessed block
		 */
		void checkCapacity(number block);

		public:
		/**
		 * @brief Construct a new In Memory Position Map Adapter object
		 *
		 * @param capacity maximum capacity (defines the allocated array size)
		 */
		InMemoryPositionMapAdapter(number capacity);

		~InMemoryPositionMapAdapter() final;
		number get(number block) final;
		void set(number block, number leaf) final;

		/**
		 * @brief write state to a binary file
		 *
		 * @param filename the name of the file to write to
		 */
		void storeToFile(string filename);

		/**
		 * @brief read state from a binary file
		 *
		 * @param filename the name of the file to read from
		 */
		void loadFromFile(string filename);
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
		shared_ptr<ORAM> oram;

		friend class ORAMBigTest;

		public:
		/**
		 * @brief Construct a new ORAMPositionMapAdapter object
		 *
		 * @param oram the intialized (with proper capacities) ORAM that will be used as a position map storage
		 */
		ORAMPositionMapAdapter(shared_ptr<ORAM> oram);
		~ORAMPositionMapAdapter() final;
		number get(number block) final;
		void set(number block, number leaf) final;
	};
}
