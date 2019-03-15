#include "definitions.h"
#include "max-flow.hpp"

#include "gtest/gtest.h"

using namespace std;

class Test : public ::testing::Test
{
	private:
	protected:
	Test()
	{
	}

	~Test() override
	{
	}
};

TEST_F(Test, HelloWorld)
{
	// pass
}
int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
