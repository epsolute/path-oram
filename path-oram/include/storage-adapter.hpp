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
		ulong capacity;
		ulong blockSize;

		void checkCapacity(ulong id);
		void checkBlockSize(ulong dataSize);

		public:
		InMemoryStorageAdapter(ulong capacity, ulong blockSize);
		~InMemoryStorageAdapter() final;
		bytes get(ulong id) final;
		void set(ulong id, bytes data) final;
	};
}
