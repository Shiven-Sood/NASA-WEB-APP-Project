#pragma once
#include "telemetry_parser.h"

struct KDNode {
    FireData data;
    KDNode *left, *right;
};

class SpatialIndex {
public:
    KDNode* buildKDTree(std::vector<FireData>& points, int depth = 0);
    // Hilbert Space-Filling Curve mapping for 3D coordinates
    uint64_t calculateHilbertDistance(double x, double y, double z);
};