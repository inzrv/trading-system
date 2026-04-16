#pragma once

#include <cstdint>

namespace metrics
{

struct Snapshot
{
    uint64_t ws_messages_total{0};
    uint64_t ws_reconnects_total{0};
    uint64_t snapshot_requests_total{0};
    uint64_t snapshot_request_failures_total{0};
    uint64_t decode_errors_total{0};
    uint64_t sequencer_gap_detected_total{0};
    uint64_t recoveries_total{0};
    uint64_t queue_drops_total{0};
    uint64_t orderbook_invalid_total{0};

    uint64_t queue_size{0};
    uint64_t queue_high_watermark{0};
    uint64_t last_applied_update_id{0};
    bool is_recovering{false};

    int64_t last_ws_message_unix_ms{0};
    int64_t last_snapshot_unix_ms{0};
    int64_t last_applied_update_unix_ms{0};
};

} // namespace metrics
