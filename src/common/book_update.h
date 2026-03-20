#pragma once

#include "level.h"
#include "symbol.h"

#include <chrono>
#include <vector>

struct SequencedBookUpdate
{
    std::chrono::system_clock::time_point event_time;
    Symbol symbol{Symbol::UNKNOWN};
    uint64_t first_update{0};
    uint64_t last_update{0};
    std::vector<Level> bids;
    std::vector<Level> asks;
};
