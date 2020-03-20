#include "definitions.h"
#include "solver.hpp"

#include "gtest/gtest.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

using namespace std;
using namespace MaxFlowModule;
namespace fs = boost::filesystem;

namespace MaxFlowModule
{
	class FilesTest : public ::testing::TestWithParam<string>
	{
		private:
		protected:
		// here define the values accessible inside the test methods
	};

	TEST_P(FilesTest, File)
	{
		vector<Edge> edges				= {};
		vector<WeightedVertex> vertices = {};
		long expected					= 0;

		ifstream file(GetParam());
		if (file.is_open())
		{
			string line;
			while (getline(file, line))
			{
				long from, to, weight, identifier;

				if (line.empty())
				{
					continue;
				}
				else if (sscanf(line.c_str(), "%ld %ld %ld", &from, &to, &weight) == 3)
				{
					edges.push_back(Edge{from, to, weight});
				}
				else if (sscanf(line.c_str(), "%ld %ld", &identifier, &weight) == 2)
				{
					vertices.push_back(WeightedVertex{identifier, weight});
				}
				else
				{
					sscanf(line.c_str(), "%ld", &expected);
				}
			}
			file.close();
		}
		else
		{
			FAIL() << "File " << GetParam() << " could not be opened.";
		}

		auto amplifier = 1000;

		auto* solver = new Solver(edges, vertices);
		solver->amplify(amplifier);
		auto [value, flow, alpha] = solver->solve(1.0, 0.01);

		ASSERT_NEAR(expected * amplifier, value, amplifier);
		ASSERT_NE(0, flow.size());
		ASSERT_GT(1, alpha);
		ASSERT_LT(0, alpha);
	}

	vector<string> readFiles()
	{
		vector<string> result = {};
		string path			  = "./resources/test-files";
		for (const auto& entry : fs::directory_iterator(path))
		{
			result.push_back(entry.path().string());
		}
		return result;
	};
	string printName(testing::TestParamInfo<string> input)
	{
		auto name = input.param.substr(input.param.find_last_of("/") + 1);
		name	  = name.substr(0, name.find_last_of("."));
		boost::replace_all(name, "-", "");
		boost::replace_all(name, "_", "");
		return name;
	}

	INSTANTIATE_TEST_SUITE_P(AllFiles, FilesTest, ::testing::ValuesIn(readFiles()), printName);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
