#include "max-flow.hpp"

#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
	cout << "Hello, world" << endl;

	MaxFlow *mf = new MaxFlow(0, 0, 0);
	mf->flow();

	exit(0);
}
