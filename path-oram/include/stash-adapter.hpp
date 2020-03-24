#include "definitions.h"

#include <iostream>
#include <unordered_map>

namespace PathORAM
{
	using namespace std;

	class AbsStashAdapter
	{
		public:
		virtual unordered_map<ulong, bytes> getAll() = 0;
		virtual void add(ulong block, bytes data)	= 0;
		virtual void update(ulong block, bytes data) = 0;
		virtual bytes get(ulong block)				 = 0;
		virtual void remove(ulong block)			 = 0;

		virtual ~AbsStashAdapter() = 0;
	};

	class InMemoryStashAdapter : public AbsStashAdapter
	{
		private:
		unordered_map<ulong, bytes> stash;
		ulong capacity;

		void checkOverflow(ulong block);

		public:
		InMemoryStashAdapter(ulong capacity);
		~InMemoryStashAdapter() final;

		unordered_map<ulong, bytes> getAll() final;
		void add(ulong block, bytes data) final;
		void update(ulong block, bytes data) final;
		bytes get(ulong block) final;
		void remove(ulong block) final;
	};
}
