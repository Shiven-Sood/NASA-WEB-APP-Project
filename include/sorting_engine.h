#pragma once
#include <vector>
#include <cstdint>
#include <string>

struct FireData {
    double oxygen_concentration;
    double atmospheric_pressure;
    double flame_spread_rate;
    uint32_t spatial_index;
    uint16_t x;
    uint16_t y;
    uint16_t z;
};

struct AlgorithmMetrics {
    double radix_ms;
    double timsort_ms;
    double mergesort_ms;
    int kdtree_nodes;
    int rtree_boxes;
    int hashtable_buckets;
};

void radixSortHilbert(std::vector<FireData>& dataset);
AlgorithmMetrics runFullAlgorithmSuite(std::vector<FireData>& dataset);
FireData findNearestNeighborKDTree(const std::vector<FireData>& dataset, double target_o2, double target_pressure);
