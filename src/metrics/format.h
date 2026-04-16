#pragma once

#include "snapshot.h"
#include "utils/utils.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <string>

namespace metrics
{

namespace detail
{

inline int64_t age_ms(int64_t unix_ms) noexcept
{
    if (unix_ms == 0) {
        return -1;
    }

    return std::max<int64_t>(0, now_unix_ms() - unix_ms);
}

inline std::string format_age_ms(int64_t unix_ms)
{
    const auto age = age_ms(unix_ms);
    if (age < 0) {
        return "-";
    }

    return std::format("{}ms", age);
}

} // namespace detail

inline std::string format_snapshot(const Snapshot& snapshot)
{
    return std::format(
        "ws_messages={} ws_reconnects={} snapshot_requests={} snapshot_failures={} "
        "decode_errors={} sequencer_gaps={} recoveries={} queue_drops={} "
        "orderbook_invalid={} queue={}/{} last_applied_update_id={} recovering={} "
        "ws_age={} snapshot_age={} applied_age={}",
        snapshot.ws_messages_total,
        snapshot.ws_reconnects_total,
        snapshot.snapshot_requests_total,
        snapshot.snapshot_request_failures_total,
        snapshot.decode_errors_total,
        snapshot.sequencer_gap_detected_total,
        snapshot.recoveries_total,
        snapshot.queue_drops_total,
        snapshot.orderbook_invalid_total,
        snapshot.queue_size,
        snapshot.queue_high_watermark,
        snapshot.last_applied_update_id,
        snapshot.is_recovering ? "true" : "false",
        detail::format_age_ms(snapshot.last_ws_message_unix_ms),
        detail::format_age_ms(snapshot.last_snapshot_unix_ms),
        detail::format_age_ms(snapshot.last_applied_update_unix_ms));
}

} // namespace metrics
