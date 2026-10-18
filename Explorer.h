#pragma once

#include "Graph.h"
#include "Judge.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

struct Plan {
    double expectedNetProfit = 0.0;
    std::vector<char> edges;
};

class Explorer {
public:
    Explorer(Graph& graph, Judge& judge, std::size_t entrance, double dayCost)
        : graph_(graph), judge_(judge), entrance_(entrance), current_(entrance),
          dayCost_(dayCost), explored_(graph.vertexCount(), false) {
        explored_[entrance_ - 1] = true;
    }

    ExpeditionResult run() {
        while (true) {
            std::vector<char> swept(graph_.vertexCount(), false);
            exploreReachable(entrance_, swept);

            const Plan plan = makePlan();
            const int frontierEdge = chooseFrontierEdge(plan);
            if (frontierEdge == -1) {
                break;
            }

            const std::size_t edgeId = static_cast<std::size_t>(frontierEdge);
            const Edge& e = graph_.edge(edgeId);
            const std::size_t source = explored_[e.a - 1] ? e.a : e.b;
            moveTo(source);

            for (const LookItem& item : judge_.LOOK()) {
                graph_.observe(item.edgeId, item.intact);
            }
            if (graph_.edge(edgeId).knowledge != EdgeKnowledge::Broken) {
                returnToEntrance();
                continue;
            }

            judge_.CLEAR(edgeId);
            graph_.markCleared(edgeId);
            current_ = entrance_;
        }

        const ExpeditionResult result = judge_.END();
        return result;
    }

private:
    struct Path {
        bool exists = false;
        double expectedDays = 0.0;
        std::vector<std::size_t> edges;
        std::vector<std::size_t> vertices;
    };

    void exploreReachable(std::size_t vertex, std::vector<char>& swept) {
        swept[vertex - 1] = true;
        current_ = vertex;

        const std::vector<LookItem> observations = judge_.LOOK();
        for (const LookItem& item : observations) {
            graph_.observe(item.edgeId, item.intact);
        }

        for (const LookItem& item : observations) {
            if (!item.intact || swept[item.destination - 1]) {
                continue;
            }

            if (!judge_.MOVE(item.edgeId)) {
                graph_.observe(item.edgeId, false);
                continue;
            }

            current_ = item.destination;
            explored_[item.destination - 1] = true;
            exploreReachable(item.destination, swept);

            judge_.MOVE(item.edgeId);
            current_ = vertex;
        }
    }

    double edgeDays(std::size_t edgeId) const {
        return graph_.expectedClearDays(edgeId);
    }

    Path shortestPathTo(std::size_t target,
                        const std::vector<char>& plannedEdges) const {
        const std::size_t n = graph_.vertexCount();
        const double infinity = std::numeric_limits<double>::infinity();
        std::vector<double> distance(n, infinity);
        std::vector<int> parentVertex(n, -1);
        std::vector<int> parentEdge(n, -1);
        using QueueItem = std::pair<double, std::size_t>;
        std::priority_queue<QueueItem, std::vector<QueueItem>,
                            std::greater<QueueItem>> queue;

        for (std::size_t vertex = 1; vertex <= n; ++vertex) {
            if (explored_[vertex - 1]) {
                distance[vertex - 1] = 0.0;
                parentEdge[vertex - 1] = -2;
                queue.push({0.0, vertex});
            }
        }

        while (!queue.empty()) {
            const auto [knownDistance, vertex] = queue.top();
            queue.pop();
            if (knownDistance > distance[vertex - 1] + 1e-12) {
                continue;
            }

            for (std::size_t edgeId : graph_.incidentEdges(vertex)) {
                const std::size_t next = graph_.otherEndpoint(edgeId, vertex);
                const double weight = plannedEdges[edgeId - 1]
                    ? 0.0 : edgeDays(edgeId);
                const double candidate = knownDistance + weight;
                if (candidate + 1e-12 < distance[next - 1]) {
                    distance[next - 1] = candidate;
                    parentVertex[next - 1] = static_cast<int>(vertex);
                    parentEdge[next - 1] = static_cast<int>(edgeId);
                    queue.push({candidate, next});
                }
            }
        }

        if (!std::isfinite(distance[target - 1])) {
            return {};
        }

        Path path;
        path.exists = true;
        path.expectedDays = distance[target - 1];
        std::size_t vertex = target;
        path.vertices.push_back(vertex);
        while (parentEdge[vertex - 1] != -2) {
            const int edgeId = parentEdge[vertex - 1];
            if (edgeId == -1) {
                return {};
            }
            path.edges.push_back(static_cast<std::size_t>(edgeId));
            vertex = static_cast<std::size_t>(parentVertex[vertex - 1]);
            path.vertices.push_back(vertex);
        }
        std::reverse(path.edges.begin(), path.edges.end());
        std::reverse(path.vertices.begin(), path.vertices.end());
        return path;
    }

    Plan makePlan() const {
        const std::size_t n = graph_.vertexCount();
        const std::size_t m = graph_.edgeCount();
        std::vector<char> plannedEdges(m, false);
        std::vector<char> plannedVertices(n, false);
        double plannedDays = 0.0;
        int plannedReward = 0;

        Plan best;
        best.edges.assign(m, false);

        for (std::size_t step = 0; step < n; ++step) {
            int bestTarget = -1;
            Path bestPath;
            double bestRatio = -1.0;

            for (std::size_t target = 1; target <= n; ++target) {
                if (explored_[target - 1] || plannedVertices[target - 1]) {
                    continue;
                }
                const int reward = graph_.profit(target);
                if (reward <= 0) {
                    continue;
                }

                Path path = shortestPathTo(target, plannedEdges);
                if (!path.exists) {
                    continue;
                }

                double marginalDays = 0.0;
                for (std::size_t edgeId : path.edges) {
                    if (!plannedEdges[edgeId - 1]) {
                        marginalDays += edgeDays(edgeId);
                    }
                }
                int marginalReward = 0;
                for (std::size_t vertex : path.vertices) {
                    if (!explored_[vertex - 1] && !plannedVertices[vertex - 1]) {
                        marginalReward += graph_.profit(vertex);
                    }
                }
                const double marginalCost = dayCost_ * marginalDays;
                double ratio;
                if (marginalCost <= 1e-12) {
                    ratio = marginalReward > 0
                        ? std::numeric_limits<double>::infinity()
                        : marginalReward < 0
                            ? -std::numeric_limits<double>::infinity()
                            : 0.0;
                } else {
                    ratio = marginalReward / marginalCost;
                }

                if (bestTarget == -1 || ratio > bestRatio + 1e-12) {
                    bestTarget = static_cast<int>(target);
                    bestRatio = ratio;
                    bestPath = std::move(path);
                }
            }

            if (bestTarget == -1) {
                break;
            }

            for (std::size_t vertex : bestPath.vertices) {
                if (!explored_[vertex - 1] && !plannedVertices[vertex - 1]) {
                    plannedVertices[vertex - 1] = true;
                    plannedReward += graph_.profit(vertex);
                }
            }
            for (std::size_t edgeId : bestPath.edges) {
                if (!plannedEdges[edgeId - 1]) {
                    plannedEdges[edgeId - 1] = true;
                    plannedDays += edgeDays(edgeId);
                }
            }

            const double expectedNet = plannedReward - dayCost_ * plannedDays;
            if (expectedNet > best.expectedNetProfit + 1e-12) {
                best.expectedNetProfit = expectedNet;
                best.edges = plannedEdges;
            }
        }
        return best;
    }

    int chooseFrontierEdge(const Plan& plan) const {
        int chosen = -1;
        std::size_t bestDistance = std::numeric_limits<std::size_t>::max();
        std::vector<std::size_t> safeDistance(
            graph_.vertexCount(), std::numeric_limits<std::size_t>::max());
        std::queue<std::size_t> queue;
        safeDistance[entrance_ - 1] = 0;
        queue.push(entrance_);
        while (!queue.empty()) {
            const std::size_t vertex = queue.front();
            queue.pop();
            for (std::size_t edgeId : graph_.incidentEdges(vertex)) {
                const EdgeKnowledge status = graph_.edge(edgeId).knowledge;
                if (status != EdgeKnowledge::Open && status != EdgeKnowledge::Cleared) {
                    continue;
                }
                const std::size_t next = graph_.otherEndpoint(edgeId, vertex);
                if (safeDistance[next - 1] ==
                    std::numeric_limits<std::size_t>::max()) {
                    safeDistance[next - 1] = safeDistance[vertex - 1] + 1;
                    queue.push(next);
                }
            }
        }

        for (std::size_t edgeId = 1; edgeId <= graph_.edgeCount(); ++edgeId) {
            if (!plan.edges[edgeId - 1] ||
                graph_.edge(edgeId).knowledge != EdgeKnowledge::Broken) {
                continue;
            }
            const Edge& e = graph_.edge(edgeId);
            const bool aExplored = explored_[e.a - 1];
            const bool bExplored = explored_[e.b - 1];
            if (aExplored == bExplored) {
                continue;
            }
            const std::size_t source = aExplored ? e.a : e.b;
            const std::size_t distance = safeDistance[source - 1];
            if (distance < bestDistance ||
                (distance == bestDistance &&
                 (chosen == -1 || edgeId < static_cast<std::size_t>(chosen)))) {
                chosen = static_cast<int>(edgeId);
                bestDistance = distance;
            }
        }
        return chosen;
    }

    std::vector<std::size_t> safeRoute(std::size_t target) const {
        const std::size_t n = graph_.vertexCount();
        std::vector<int> parentVertex(n, -1);
        std::vector<int> parentEdge(n, -1);
        std::queue<std::size_t> queue;
        parentVertex[entrance_ - 1] = static_cast<int>(entrance_);
        queue.push(entrance_);

        while (!queue.empty() && parentVertex[target - 1] == -1) {
            const std::size_t vertex = queue.front();
            queue.pop();
            for (std::size_t edgeId : graph_.incidentEdges(vertex)) {
                const EdgeKnowledge status = graph_.edge(edgeId).knowledge;
                if (status != EdgeKnowledge::Open && status != EdgeKnowledge::Cleared) {
                    continue;
                }
                const std::size_t next = graph_.otherEndpoint(edgeId, vertex);
                if (parentVertex[next - 1] == -1) {
                    parentVertex[next - 1] = static_cast<int>(vertex);
                    parentEdge[next - 1] = static_cast<int>(edgeId);
                    queue.push(next);
                }
            }
        }

        std::vector<std::size_t> route;
        if (parentVertex[target - 1] == -1) {
            return route;
        }
        for (std::size_t vertex = target; vertex != entrance_;
             vertex = static_cast<std::size_t>(parentVertex[vertex - 1])) {
            route.push_back(static_cast<std::size_t>(parentEdge[vertex - 1]));
        }
        std::reverse(route.begin(), route.end());
        return route;
    }

    void moveTo(std::size_t target) {
        for (std::size_t edgeId : safeRoute(target)) {
            if (!judge_.MOVE(edgeId)) {
                graph_.observe(edgeId, false);
                return;
            }
            current_ = graph_.otherEndpoint(edgeId, current_);
        }
    }

    void returnToEntrance() {
        std::vector<std::size_t> route = safeRoute(current_);
        std::reverse(route.begin(), route.end());
        for (std::size_t edgeId : route) {
            if (!judge_.MOVE(edgeId)) {
                graph_.observe(edgeId, false);
                return;
            }
            current_ = graph_.otherEndpoint(edgeId, current_);
        }
    }

    Graph& graph_;
    Judge& judge_;
    std::size_t entrance_;
    std::size_t current_;
    double dayCost_;
    std::vector<char> explored_;
};
