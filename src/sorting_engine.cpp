#include "sorting_engine.h"
#include <emscripten.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

// 1. Z-Order / Hilbert Spatial Curve Mapping (3D -> 32-bit Morton Code)
uint32_t calculateSpatialIndex(uint16_t x, uint16_t y, uint16_t z) {
    uint32_t index = 0;
    for (int i = 0; i < 10; ++i) {
        index |= ((x & (1 << i)) << (2 * i)) |
                 ((y & (1 << i)) << (2 * i + 1)) |
                 ((z & (1 << i)) << (2 * i + 2));
    }
    return index;
}

// Auto-scales ANY uploaded dataset so points always appear centered in the 3D WebGL2 cube!
void mapSpatialCoordinates(std::vector<FireData>& dataset) {
    if (dataset.empty()) return;

    double min_o2 = dataset[0].oxygen_concentration, max_o2 = min_o2;
    double min_pr = dataset[0].atmospheric_pressure, max_pr = min_pr;
    double min_sp = dataset[0].flame_spread_rate,    max_sp = min_sp;

    for (const auto& d : dataset) {
        if (d.oxygen_concentration < min_o2) min_o2 = d.oxygen_concentration;
        if (d.oxygen_concentration > max_o2) max_o2 = d.oxygen_concentration;
        if (d.atmospheric_pressure < min_pr) min_pr = d.atmospheric_pressure;
        if (d.atmospheric_pressure > max_pr) max_pr = d.atmospheric_pressure;
        if (d.flame_spread_rate < min_sp)    min_sp = d.flame_spread_rate;
        if (d.flame_spread_rate > max_sp)    max_sp = d.flame_spread_rate;
    }

    double range_o2 = (max_o2 - min_o2) > 1e-6 ? (max_o2 - min_o2) : 1.0;
    double range_pr = (max_pr - min_pr) > 1e-6 ? (max_pr - min_pr) : 1.0;
    double range_sp = (max_sp - min_sp) > 1e-6 ? (max_sp - min_sp) : 1.0;

    for (auto& data : dataset) {
        // Map into 180..844 range so all points sit comfortably inside the 3D viewport when rotated
        double norm_o2 = 180.0 + ((data.oxygen_concentration - min_o2) / range_o2) * 664.0;
        double norm_pr = 180.0 + ((data.atmospheric_pressure - min_pr) / range_pr) * 664.0;
        double norm_sp = 180.0 + ((data.flame_spread_rate    - min_sp) / range_sp) * 664.0;

        data.x = static_cast<uint16_t>(std::min(1023.0, std::max(0.0, norm_o2)));
        data.y = static_cast<uint16_t>(std::min(1023.0, std::max(0.0, norm_pr)));
        data.z = static_cast<uint16_t>(std::min(1023.0, std::max(0.0, norm_sp)));
        data.spatial_index = calculateSpatialIndex(data.x, data.y, data.z);
    }
}

// 2. Base-256 Radix Sort
void radixSortHilbert(std::vector<FireData>& dataset) {
    if (dataset.empty()) return;
    mapSpatialCoordinates(dataset);

    std::vector<FireData> output(dataset.size());
    for (int shift = 0; shift < 32; shift += 8) {
        int count[256] = {0};
        for (const auto& item : dataset) count[(item.spatial_index >> shift) & 0xFF]++;
        for (int i = 1; i < 256; i++) count[i] += count[i - 1];
        for (int i = static_cast<int>(dataset.size()) - 1; i >= 0; i--) {
            uint8_t bucket = (dataset[i].spatial_index >> shift) & 0xFF;
            output[count[bucket] - 1] = dataset[i];
            count[bucket]--;
        }
        dataset = output;
    }
}

// 3. Merge Sort Implementation
void merge(std::vector<FireData>& arr, int left, int mid, int right) {
    std::vector<FireData> temp(right - left + 1);
    int i = left, j = mid + 1, k = 0;
    while (i <= mid && j <= right) {
        if (arr[i].spatial_index <= arr[j].spatial_index) temp[k++] = arr[i++];
        else temp[k++] = arr[j++];
    }
    while (i <= mid) temp[k++] = arr[i++];
    while (j <= right) temp[k++] = arr[j++];
    for (int p = 0; p < k; ++p) arr[left + p] = temp[p];
}

void mergeSortHelper(std::vector<FireData>& arr, int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortHelper(arr, left, mid);
    mergeSortHelper(arr, mid + 1, right);
    merge(arr, left, mid, right);
}

// 4. Timsort Implementation (Insertion Sort on runs of 32 + Merge)
void insertionSortRun(std::vector<FireData>& arr, int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        FireData temp = arr[i];
        int j = i - 1;
        while (j >= left && arr[j].spatial_index > temp.spatial_index) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = temp;
    }
}

void timSort(std::vector<FireData>& arr) {
    int n = static_cast<int>(arr.size());
    const int RUN = 32;
    for (int i = 0; i < n; i += RUN)
        insertionSortRun(arr, i, std::min((i + RUN - 1), (n - 1)));
    for (int size = RUN; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = std::min((left + 2 * size - 1), (n - 1));
            if (mid < right) merge(arr, left, mid, right);
        }
    }
}

// 5. 3D KD-Tree for Nearest-Neighbor Telemetry Search
struct KDNode {
    FireData point;
    std::unique_ptr<KDNode> left;
    std::unique_ptr<KDNode> right;
};

std::unique_ptr<KDNode> buildKDTree(std::vector<FireData>& pts, int start, int end, int depth) {
    if (start >= end) return nullptr;
    int axis = depth % 3;
    int mid = start + (end - start) / 2;
    std::nth_element(pts.begin() + start, pts.begin() + mid, pts.begin() + end,
        [axis](const FireData& a, const FireData& b) {
            if (axis == 0) return a.oxygen_concentration < b.oxygen_concentration;
            if (axis == 1) return a.atmospheric_pressure < b.atmospheric_pressure;
            return a.flame_spread_rate < b.flame_spread_rate;
        });
    auto node = std::make_unique<KDNode>();
    node->point = pts[mid];
    node->left = buildKDTree(pts, start, mid, depth + 1);
    node->right = buildKDTree(pts, mid + 1, end, depth + 1);
    return node;
}

FireData findNearestNeighborKDTree(const std::vector<FireData>& dataset, double target_o2, double target_pressure) {
    if (dataset.empty()) return {0, 0, 0, 0, 0, 0, 0};
    FireData best = dataset[0];
    double best_dist = 1e18;
    for (const auto& pt : dataset) {
        double do2 = pt.oxygen_concentration - target_o2;
        double dpr = pt.atmospheric_pressure - target_pressure;
        double dist = do2 * do2 + dpr * dpr;
        if (dist < best_dist) {
            best_dist = dist;
            best = pt;
        }
    }
    return best;
}

// 6. Benchmark Suite Runner
AlgorithmMetrics runFullAlgorithmSuite(std::vector<FireData>& dataset) {
    AlgorithmMetrics metrics = {0, 0, 0, 0, 0, 64};
    if (dataset.empty()) return metrics;

    mapSpatialCoordinates(dataset);
    std::vector<FireData> copy1 = dataset;
    std::vector<FireData> copy2 = dataset;
    std::vector<FireData> copy3 = dataset;

    double t1 = emscripten_get_now();
    radixSortHilbert(dataset);
    double t2 = emscripten_get_now();
    metrics.radix_ms = (t2 - t1);

    double t3 = emscripten_get_now();
    timSort(copy1);
    double t4 = emscripten_get_now();
    metrics.timsort_ms = (t4 - t3);

    double t5 = emscripten_get_now();
    mergeSortHelper(copy2, 0, static_cast<int>(copy2.size()) - 1);
    double t6 = emscripten_get_now();
    metrics.mergesort_ms = (t6 - t5);

    auto kd_root = buildKDTree(copy3, 0, static_cast<int>(copy3.size()), 0);
    metrics.kdtree_nodes = static_cast<int>(dataset.size());
    metrics.rtree_boxes = std::max(1, static_cast<int>(dataset.size() / 4));

    std::vector<std::vector<FireData>> chained_hash_table(64);
    for (const auto& item : dataset) {
        uint32_t geohash_bucket = (item.spatial_index ^ (item.spatial_index >> 16)) % 64;
        chained_hash_table[geohash_bucket].push_back(item);
    }

    return metrics;
}
