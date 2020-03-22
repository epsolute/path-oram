#include "definitions.h"

#include <iostream>
#include <unordered_map>

namespace PathORAM
{
	using namespace std;

	class AbsStashAdapter
	{
		public:
		virtual iterator<forward_iterator_tag, pair<ulong, bytes>> begin() = 0;
		virtual iterator<forward_iterator_tag, pair<ulong, bytes>> end()   = 0;
		virtual void add(pair<ulong, bytes> block)						   = 0;
		virtual void remove(pair<ulong, bytes> block)					   = 0;
		virtual ~AbsStashAdapter()										   = 0;
	};

	class InMemoryStashAdapter : AbsStashAdapter
	{
		private:
		unordered_map<ulong, bytes> stash;
		ulong capacity;

		public:
		InMemoryStashAdapter(ulong capacity);
		~InMemoryStashAdapter() final;

		unordered_map<ulong, bytes>::iterator begin() final;
		void add(pair<ulong, bytes> block) final;
		void remove(pair<ulong, bytes> block) final;
	};
}
