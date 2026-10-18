#pragma once

#include "Graph.h"

#include <cstdint>
#include <random>
#include <vector>

struct LookItem {
    std::size_t edgeId;
    std::size_t destination;
    bool intact;
};

struct ExpeditionResult {
    int collectedProfit;
    std::size_t days;
    double netProfit;
};

class Judge {
public:
    Judge(const Graph& map, std::size_t entrance, double dayCost,
          std::uint64_t seed = std::random_device{}())
        : map_(map), entrance_(entrance), current_(entrance),
          dayCost_(dayCost), intact_(map.edgeCount(), false),
          cleared_(map.edgeCount(), false), collected_(map.vertexCount(), false),
          rng_(seed) {
        const long double range =
            static_cast<long double>(std::mt19937_64::max()) + 1.0L;
        for (std::size_t edgeId = 1; edgeId <= map_.edgeCount(); ++edgeId) {
            const long double sample = static_cast<long double>(rng_()) / range;
            intact_[edgeId - 1] = sample < map_.edge(edgeId).p;
        }
        enter(entrance_);
    }

    std::vector<LookItem> LOOK() const {
        std::vector<LookItem> answer;
        for (std::size_t edgeId : map_.incidentEdges(current_)) {
            answer.push_back({edgeId, map_.otherEndpoint(edgeId, current_),
                              intact_[edgeId - 1] || cleared_[edgeId - 1]});
        }
        return answer;
    }

    bool MOVE(std::size_t edgeId) {
        if (!(intact_[edgeId - 1] || cleared_[edgeId - 1])) {
            return false;
        }
        current_ = map_.otherEndpoint(edgeId, current_);
        enter(current_);
        return true;
    }

    void CLEAR(std::size_t edgeId) {
        cleared_[edgeId - 1] = true;
        ++days_;
        current_ = entrance_;
    }

    ExpeditionResult END() const {
        return {collectedProfit_, days_, collectedProfit_ - days_ * dayCost_};
    }

    std::size_t currentVertex() const {
        return current_;
    }

private:
    void enter(std::size_t vertex) {
        if (!collected_[vertex - 1]) {
            collected_[vertex - 1] = true;
            collectedProfit_ += map_.profit(vertex);
        }
    }

    Graph map_;
    std::size_t entrance_;
    std::size_t current_;
    double dayCost_;
    std::vector<bool> intact_;
    std::vector<bool> cleared_;
    std::vector<bool> collected_;
    int collectedProfit_ = 0;
    std::size_t days_ = 0;
    std::mt19937_64 rng_;
};
