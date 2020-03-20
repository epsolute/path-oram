#include "definitions.h"

#include <iostream>

namespace PathORAM
{
	class StorageAdapter
	{
		private:
		long result = -1;

		public:
		StorageAdapter(std::vector<Edge> edges, long s, long t);

		long flowValue();
	};
}
