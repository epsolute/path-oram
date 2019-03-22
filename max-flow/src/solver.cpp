#include "solver.hpp"

#include "max-flow.hpp"

namespace MaxFlowModule
{
	Solver::Solver(vector<Edge> edges, vector<WeightedVertex> vertices)
	{
		long largestVertexIndex = min_element(
									  vertices.begin(),
									  vertices.end(),
									  [](WeightedVertex v1, WeightedVertex v2) { return v1.identifier < v2.identifier; })
									  ->identifier;

		this->nVertices = vertices.size() + 2;

		this->originalEdges = edges;

		this->source		   = largestVertexIndex + 1;
		this->source		   = largestVertexIndex + 2;
		this->addedSourceEdges = vector<Edge>{};
		this->addedSinkEdges   = vector<Edge>{};

		for (auto const& vertex : vertices)
		{
			if (vertex.weight <= 0)
			{
				this->addedSourceEdges.push_back(Edge{source, vertex.identifier, vertex.weight});
			}
			else
			{
				this->addedSinkEdges.push_back(Edge{source, vertex.identifier, vertex.weight});
			}
		}
	}

	std::tuple<double, vector<Flow>> Solver::solve(double maxAlpha, double precisionEpsilon)
	{
		double L		   = 0;
		double R		   = maxAlpha;
		double resultAlpha = (L + R) * 0.5;

		do
		{
			double seedAlpha = (L + R) * 0.5;

			MaxFlow* mf		  = new MaxFlow(this->constructEdges(seedAlpha), this->nVertices, this->source, this->sink);
			vector<Flow> flow = mf->flow();

			if (saturatedSource(flow, this->addedSourceEdges, seedAlpha) < (1 - precisionEpsilon)) // switched to cut inside the graph
			{
				L			= seedAlpha;
				resultAlpha = seedAlpha;
			}
			else
			{
				R = seedAlpha;
			}

		} while ((R - L) > precisionEpsilon);

		MaxFlow* mf = new MaxFlow(this->constructEdges(resultAlpha), this->nVertices, this->source, this->sink);

		return make_tuple(mf->flowValue(), mf->flow());
	}

	vector<Edge> Solver::constructEdges(double seedAlpha)
	{
		vector<Edge> edges{};
		edges.resize(this->addedSourceEdges.size());

		transform(this->addedSourceEdges.begin(), this->addedSourceEdges.end(), edges.begin(), [seedAlpha](Edge e) { return Edge{e.from, e.to, (long)std::round(e.weight * seedAlpha)}; });

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
}
