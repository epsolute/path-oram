#include "definitions.h"

#include <iostream>

namespace PathORAM
{
	using namespace std;

	class AbsStorageAdapter
	{
		public:
		virtual vector<unsigned char> get(unsigned long id)			   = 0;
		virtual void set(unsigned long id, vector<unsigned char> data) = 0;
		virtual ~AbsStorageAdapter()								   = 0;
	};

	class InMemoryStorageAdapter : AbsStorageAdapter
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
		vector<unsigned char> get(unsigned long id) final;
		void set(unsigned long id, vector<unsigned char> data) final;
	};
}
