#include "definitions.h"

#include <iostream>

namespace PathORAM
{
	using namespace std;

	class AbsStorageAdapter
	{
		public:
		virtual bytes get(ulong id)			   = 0;
		virtual void set(ulong id, bytes data) = 0;
		virtual ~AbsStorageAdapter()		   = 0;
	};

	class InMemoryStorageAdapter : public AbsStorageAdapter
	{
		private:
		unsigned char **blocks;
		unsigned int capacity;
		unsigned int blockSize;

		void checkCapacity(unsigned int id);
		void checkBlockSize(unsigned int dataSize);

		public:
		InMemoryStorageAdapter(unsigned int capacity, unsigned int blockSize);
		~InMemoryStorageAdapter() final;
		bytes get(ulong id) final;
		void set(ulong id, bytes data) final;
	};
}
