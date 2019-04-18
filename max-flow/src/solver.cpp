#include "solver.hpp"

#include "max-flow.hpp"

#include <iostream>

#ifdef SHARED
extern "C"
{
#endif
	MaxFlowModule::Solution maxFlow(int nEdges, MaxFlowModule::Edge* edges, int nVertices, MaxFlowModule::WeightedVertex* vertices, double alpha, double epsilon)
	{
		std::vector<MaxFlowModule::Edge> edgesVec(edges, edges + nEdges);
		std::vector<MaxFlowModule::WeightedVertex> verticesVec(vertices, vertices + nVertices);

		auto* solver					= new MaxFlowModule::Solver(edgesVec, verticesVec);
		auto [value, flow, alphaResult] = solver->solve(alpha, epsilon);

		// a hack to mitigate python c_types bug
		flow.insert(flow.begin(), MaxFlowModule::Flow{1, 0, 3});

		return MaxFlowModule::Solution{value, &flow[0], (long)flow.size(), alphaResult};
	}
#ifdef SHARED
}
#endif

namespace MaxFlowModule
{
	Solver::Solver(vector<Edge> edges, vector<WeightedVertex> vertices)
	{
		if (edges.size() == 0 || vertices.size() == 0)
		{
			throw "Empty input";
		}

		long largestVertexIndex = max_element(
									  vertices.begin(),
									  vertices.end(),
									  [](WeightedVertex v1, WeightedVertex v2) { return v1.identifier < v2.identifier; })
									  ->identifier;

		this->originalEdges = edges;

		this->source		   = largestVertexIndex + 1;
		this->sink			   = largestVertexIndex + 2;
		this->addedSourceEdges = vector<Edge>{};
		this->addedSinkEdges   = vector<Edge>{};

		for (auto const& vertex : vertices)
		{
			if (vertex.weight < 0)
			{
				this->addedSourceEdges.push_back(Edge{source, vertex.identifier, -vertex.weight});
			}
			else if (vertex.weight > 0)
			{
				this->addedSinkEdges.push_back(Edge{vertex.identifier, sink, vertex.weight});
			}
		}
	}

	std::tuple<long, vector<Flow>, double> Solver::solve(double maxAlpha, double precisionEpsilon)
	{
		double L		   = 0;
		double R		   = maxAlpha;
		double resultAlpha = (L + R) * 0.5;

		do
		{
			double seedAlpha = (L + R) * 0.5;

			MaxFlow* mf		  = new MaxFlow(this->constructEdges(seedAlpha), this->source, this->sink);
			vector<Flow> flow = mf->flow();

			auto saturation = saturatedSource(flow, this->addedSourceEdges, seedAlpha);
			// cout << "alpha: " << seedAlpha << ", saturation: " << saturation << endl;

			if (saturation < (1 - precisionEpsilon)) // e.g. 0.5, source edges are NOT saturated
			{										 // decrease alpha, decrease source edges capacity
				R			= seedAlpha;
				resultAlpha = seedAlpha;
			}
			else // source edges are saturated
			{	// increase source edges capacity
				L = seedAlpha;
			}

		} while ((R - L) > precisionEpsilon);

		MaxFlow* mf = new MaxFlow(this->constructEdges(resultAlpha), this->source, this->sink);

		auto result = make_tuple(mf->flowValue(), mf->flow(), resultAlpha);
		return result;
	}

	vector<Edge> Solver::constructEdges(double seedAlpha)
	{
		vector<Edge> edges = {};
		transform(
			this->addedSourceEdges.begin(),
			this->addedSourceEdges.end(),
			std::back_inserter(edges),
			[seedAlpha](Edge e) { return Edge{e.from, e.to, (long)std::round(e.weight * seedAlpha)}; });

		edges.insert(edges.end(), this->originalEdges.begin(), this->originalEdges.end());
		edges.insert(edges.end(), this->addedSinkEdges.begin(), this->addedSinkEdges.end());

		return edges;
	}

	double Solver::saturatedSource(vector<Flow> flow, vector<Edge> edges, double alpha)
	{
		double saturated = 0;
		double total	 = 0;

		for (auto const& edge : flow)
		{
			if (edge.from == this->source)
			{
				saturated += edge.saturation;
			}
		}

		for (auto const& edge : edges)
		{
			if (edge.from == this->source)
			{
				total += std::round(edge.weight * alpha);
			}
		}

		return saturated / total;
	}

	void Solver::amplify(long amplifier)
	{
		for (int i = 0; i < this->originalEdges.size(); i++)
		{
			this->originalEdges[i].weight *= amplifier;
		}

		for (int i = 0; i < this->addedSourceEdges.size(); i++)
		{
			this->addedSourceEdges[i].weight *= amplifier;
		}

		for (int i = 0; i < this->addedSinkEdges.size(); i++)
		{
			this->addedSinkEdges[i].weight *= amplifier;
		}
	}
}
