#pragma once

#include <cstddef>
#include <vector>

enum class EdgeKnowledge {
    Unknown,
    Open,
    Broken,
    Cleared
};

struct Edge {
    std::size_t id;
    std::size_t a;
    std::size_t b;
    double p;
    EdgeKnowledge knowledge = EdgeKnowledge::Unknown;
};

struct Vertex {
    int profit = 0;
    std::vector<std::size_t> incidentEdges;
};

class Graph {
public:
    explicit Graph(std::size_t vertexCount = 0) : vertices_(vertexCount) {}

    std::size_t vertexCount() const {
        return vertices_.size();
    }

    std::size_t edgeCount() const {
        return edges_.size();
    }

    void addEdge(std::size_t a, std::size_t b, double p) {
        const std::size_t id = edgeCount() + 1;
        edges_.push_back({id, a, b, p, EdgeKnowledge::Unknown});
        vertices_.at(a - 1).incidentEdges.push_back(id);
        vertices_.at(b - 1).incidentEdges.push_back(id);
    }

    void setProfit(std::size_t vertex, int profit) {
        vertices_.at(vertex - 1).profit = profit;
    }

    int profit(std::size_t vertex) const {
        return vertices_.at(vertex - 1).profit;
    }

    const std::vector<std::size_t>& incidentEdges(std::size_t vertex) const {
        return vertices_.at(vertex - 1).incidentEdges;
    }

    const Edge& edge(std::size_t edgeId) const {
        return edges_.at(edgeId - 1);
    }

    Edge& edge(std::size_t edgeId) {
        return edges_.at(edgeId - 1);
    }

    std::size_t otherEndpoint(std::size_t edgeId, std::size_t vertex) const {
        const Edge& e = edge(edgeId);
        return e.a == vertex ? e.b : e.a;
    }

    void observe(std::size_t edgeId, bool intact) {
        Edge& e = edge(edgeId);
        if (e.knowledge == EdgeKnowledge::Cleared) {
            return;
        }
        e.knowledge = intact ? EdgeKnowledge::Open : EdgeKnowledge::Broken;
    }

    void markCleared(std::size_t edgeId) {
        edge(edgeId).knowledge = EdgeKnowledge::Cleared;
    }

    double expectedClearDays(std::size_t edgeId) const {
        const Edge& e = edge(edgeId);
        switch (e.knowledge) {
            case EdgeKnowledge::Open:
            case EdgeKnowledge::Cleared:
                return 0.0;
            case EdgeKnowledge::Broken:
                return 1.0;
            case EdgeKnowledge::Unknown:
                return 1.0 - e.p;
        }
        return 1.0;
    }

private:
    std::vector<Vertex> vertices_;
    std::vector<Edge> edges_;
};
