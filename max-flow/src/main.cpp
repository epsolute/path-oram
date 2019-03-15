#include "max-flow.hpp"

#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
	cout << "Hello, world" << endl;

	MaxFlow *mf = new MaxFlow();
	mf->flow();

	exit(0);
}
