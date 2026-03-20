#pragma once

#include <cstdint>

enum class Side
{
    UNKNOWN,
    BID,
    ASK
};

struct Level
{
    double price{0};
    double quantity{0};
};
