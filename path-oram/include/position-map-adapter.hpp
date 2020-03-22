#include "definitions.h"

#include <iostream>

namespace PathORAM
{
	using namespace std;

	class AbsPositionMapAdapeter
	{
		public:
		virtual unsigned long get(unsigned long block)			  = 0;
		virtual void set(unsigned long block, unsigned long leaf) = 0;
		virtual ~AbsPositionMapAdapeter()						  = 0;
	};

	class InMemoryPositionMapAdapeter : AbsPositionMapAdapeter
	{
		private:
		unsigned long *map;
		unsigned int capacity;

		void checkCapacity(unsigned int block);

		public:
		InMemoryPositionMapAdapeter(unsigned int capacity);
		~InMemoryPositionMapAdapeter() final;
		unsigned long get(unsigned long block) final;
		void set(unsigned long block, unsigned long leaf) final;
	};
}
