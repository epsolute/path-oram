#ifndef POSITION_MAP_INCLUDED
#define POSITION_MAP_INCLUDED

#include "definitions.h"
#include "oram.hpp"

#include <iostream>

namespace PathORAM
{
	using namespace std;

	class AbsPositionMapAdapter
	{
		public:
		virtual ulong get(ulong block)			  = 0;
		virtual void set(ulong block, ulong leaf) = 0;
		virtual ~AbsPositionMapAdapter()		  = 0;
	};

	class InMemoryPositionMapAdapter : public AbsPositionMapAdapter
	{
		private:
		ulong *map;
		ulong capacity;

		void checkCapacity(ulong block);

		public:
		InMemoryPositionMapAdapter(ulong capacity);
		~InMemoryPositionMapAdapter() final;
		ulong get(ulong block) final;
		void set(ulong block, ulong leaf) final;
	};

	class ORAM;

	class ORAMPositionMapAdapter : public AbsPositionMapAdapter
	{
		private:
		ORAM *oram;

		public:
		ORAMPositionMapAdapter(ORAM *oram);
		~ORAMPositionMapAdapter() final;
		ulong get(ulong block) final;
		void set(ulong block, ulong leaf) final;
	};
}

#endif
