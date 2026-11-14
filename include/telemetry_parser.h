#pragma once
#include <vector>
#include <string>
#include "sorting_engine.h"

std::vector<FireData> parseTelemetryData(const std::string& csv_content);