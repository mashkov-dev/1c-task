#include "Explorer.h"

#include <iomanip>
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    double dayCost;
    std::size_t entrance;
    std::size_t vertexCount;
    std::size_t edgeCount;
    std::cin >> dayCost >> entrance >> vertexCount >> edgeCount;

    Graph map(vertexCount);
    for (std::size_t vertex = 1; vertex <= vertexCount; ++vertex) {
        int profit;
        std::cin >> profit;
        map.setProfit(vertex, profit);
    }

    for (std::size_t i = 0; i < edgeCount; ++i) {
        std::size_t a, b;
        double probability;
        std::cin >> a >> b >> probability;
        map.addEdge(a, b, probability);
    }

    Judge judge(map, entrance, dayCost);
    Explorer algorithm(map, judge, entrance, dayCost);
    const ExpeditionResult result = algorithm.run();

    std::cout << "RESULT " << std::setprecision(12) << result.collectedProfit << ' '
              << result.days << ' ' << result.netProfit << '\n';
    return 0;
}
