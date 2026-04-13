#pragma once

#include "orderbook.h"

inline bool is_valid(const L1& l1)
{
    if (!l1.best_bid || !l1.best_ask) {
        return true;
    }

    return l1.best_bid->price <= l1.best_ask->price;
}
