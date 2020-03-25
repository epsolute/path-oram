#include "definitions.h"

#include <iostream>

namespace PathORAM
{
	using namespace std;

	class AbsStorageAdapter
	{
		private:
		void checkCapacity(ulong location);
		void checkBlockSize(ulong dataSize);

		bytes key;

		public:
		pair<ulong, bytes> get(ulong location);
		void set(ulong location, pair<ulong, bytes> data);

		AbsStorageAdapter(ulong capacity, ulong userBlockSize);
		virtual ~AbsStorageAdapter() = 0;

		protected:
		ulong capacity;
		ulong blockSize;
		ulong userBlockSize;

		virtual void setInternal(ulong location, bytes raw) = 0;
		virtual bytes getInternal(ulong location)			= 0;
	};

	class InMemoryStorageAdapter : public AbsStorageAdapter
	{
		private:
		uchar **blocks;

		public:
		InMemoryStorageAdapter(ulong capacity, ulong userBlockSize);
		~InMemoryStorageAdapter() final;

		protected:
		void setInternal(ulong location, bytes raw) final;
		bytes getInternal(ulong location) final;
	};
}
