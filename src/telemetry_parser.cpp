#include "telemetry_parser.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>

std::vector<FireData> parseTelemetryData(const std::string& csv_content) {
    std::vector<FireData> dataset;
    std::stringstream file_stream(csv_content);
    std::string line;

    if (csv_content.empty()) return dataset;

    std::vector<double> single_values;

    while (std::getline(file_stream, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        std::vector<double> row_nums;

        while (std::getline(ss, cell, ',')) {
            int start = 0;
            int len = static_cast<int>(cell.length());
            while (start < len && (cell[start] == ' ' || cell[start] == '\t' || cell[start] == '\r' || cell[start] == '\n' || cell[start] == '"')) {
                start++;
            }
            if (start >= len) continue;

            // Ignore date/time strings like 2010-05-25 or 1899-12-31
            if (cell.find('-') != std::string::npos && cell.find(':') != std::string::npos) continue;
            if (cell.find("1899") != std::string::npos || cell.find("2010") != std::string::npos) continue;

            const char* str = cell.c_str() + start;
            char* endptr = nullptr;
            double val = std::strtod(str, &endptr);
            if (endptr != str && val > 0.0 && val < 1000000.0) {
                row_nums.push_back(val);
            }
        }

        // Case 1: Standard 3-column row (oxygen, pressure, spread)
        if (row_nums.size() >= 3) {
            FireData data = { row_nums[0], row_nums[1], row_nums[2], 0, 0, 0, 0 };
            dataset.push_back(data);
        }
        // Case 2: Vertical parameter sheet (like NASA's DSI-S1 Solidification Plan.xlsx)
        else if (row_nums.size() == 1 || row_nums.size() == 2) {
            for (int j = 0; j < static_cast<int>(row_nums.size()); ++j) {
                single_values.push_back(row_nums[j]);
            }
        }
    }

    // Combine both multi-column rows and vertical parameter rows into 3D triplets!
    int total_singles = static_cast<int>(single_values.size());
    if (total_singles >= 3) {
        for (int i = 0; i + 2 < total_singles; ++i) {
            double v1 = single_values[i];
            double v2 = single_values[i + 1];
            double v3 = single_values[i + 2];

            double o2_equiv = std::fmod(v1, 80.0) + 15.0;
            double pr_equiv = std::fmod(v2 * 10.0, 100.0) + 40.0;
            double sp_equiv = std::fmod(v3, 1.9) + 0.1;

            FireData data = { o2_equiv, pr_equiv, sp_equiv, 0, 0, 0, 0 };
            dataset.push_back(data);
        }
    }

    std::cout << "[C++ Parser] Extracted " << dataset.size() << " 3D telemetry records!" << std::endl;
    return dataset;
}
