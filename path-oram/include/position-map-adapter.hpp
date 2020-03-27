#pragma once

#include "definitions.h"
#include "oram.hpp"

#include <iostream>

namespace PathORAM
{
	using namespace std;

	class AbsPositionMapAdapter
	{
		public:
		virtual number get(number block)			= 0;
		virtual void set(number block, number leaf) = 0;
		virtual ~AbsPositionMapAdapter()			= 0;
	};

	class InMemoryPositionMapAdapter : public AbsPositionMapAdapter
	{
		private:
		number *map;
		number capacity;

		void checkCapacity(number block);

		public:
		InMemoryPositionMapAdapter(number capacity);
		~InMemoryPositionMapAdapter() final;
		number get(number block) final;
		void set(number block, number leaf) final;
	};

	class ORAM;

	class ORAMPositionMapAdapter : public AbsPositionMapAdapter
	{
		private:
		ORAM *oram;

		friend class ORAMBigTest;

		public:
		ORAMPositionMapAdapter(ORAM *oram);
		~ORAMPositionMapAdapter() final;
		number get(number block) final;
		void set(number block, number leaf) final;
	};
}
