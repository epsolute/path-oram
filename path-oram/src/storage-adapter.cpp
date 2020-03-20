#include "storage-adapter.hpp"
#include <vector>

namespace PathORAM
{
	using namespace boost;
	using namespace std;

	StorageAdapter::StorageAdapter(vector<Edge> edges, long s, long t)
	{
		if (edges.size() == 0 || s == t)
		{
			throw "Malformed parameters";
		}
	}
}
