#pragma once

#include "common/snapshot.h"
#include "common/book_update.h"

#include <string>

constexpr uint64_t kLastUpdateId = 100;

inline const std::string kValidBinanceSnapshot =
    R"({"lastUpdateId":100,"bids":[],"asks":[]})";

inline Snapshot make_snapshot(uint64_t last_update_id)
{
    return Snapshot{
        .last_update_id = last_update_id,
        .bids = {},
        .asks = {},
    };
}

inline SequencedBookUpdate make_update(uint64_t first_update, uint64_t last_update)
{
    return SequencedBookUpdate{
        .event_time = {},
        .symbol = Symbol::BTCUSDT,
        .first_update = first_update,
        .last_update = last_update,
        .bids = {},
        .asks = {},
    };
}

inline const std::string kValidDiffPayload = R"({
    "e":"depthUpdate",
    "E":123456789,
    "s":"BTCUSDT",
    "U":101,
    "u":102,
    "b":[["100.10","1.25"]],
    "a":[["100.20","2.50"]]
})";

inline const std::string kValidSnapshotPayload = R"({
    "lastUpdateId":100,
    "bids":[["100.10","1.25"]],
    "asks":[["100.20","2.50"]]
})";
