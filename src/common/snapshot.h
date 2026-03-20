#pragma once

#include "level.h"

#include <vector>

struct Snapshot
{
    uint64_t last_update_id{0};
    std::vector<Level> bids;
    std::vector<Level> asks;
};
