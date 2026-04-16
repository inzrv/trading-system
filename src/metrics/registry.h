#pragma once

#include "snapshot.h"
#include "utils/utils.h"

#include <atomic>
#include <chrono>
#include <cstdint>

namespace metrics
{

using counter_t = std::atomic<uint64_t>;
using gauge_t = std::atomic<uint64_t>;
using timestamp_t = std::atomic<int64_t>;
using flag_t = std::atomic<bool>;

inline void inc(counter_t& counter) noexcept
{
    counter.fetch_add(1, std::memory_order_relaxed);
}

template<typename T>
inline void store(std::atomic<T>& value, T new_value) noexcept
{
    value.store(new_value, std::memory_order_relaxed);
}

template<typename T>
inline T load(const std::atomic<T>& value) noexcept
{
    return value.load(std::memory_order_relaxed);
}

inline void update_max(gauge_t& gauge, uint64_t candidate) noexcept
{
    auto current = load(gauge);
    while (current < candidate
           && !gauge.compare_exchange_weak(
               current,
               candidate,
               std::memory_order_relaxed,
               std::memory_order_relaxed)) {
    }
}

class Registry
{
public:
    void on_ws_message() noexcept
    {
        inc(m_ws_messages_total);
        store(m_last_ws_message_unix_ms, now_unix_ms());
    }

    void on_ws_reconnect() noexcept
    {
        inc(m_ws_reconnects_total);
    }

    void on_snapshot_request() noexcept
    {
        inc(m_snapshot_requests_total);
        store(m_last_snapshot_unix_ms, now_unix_ms());
    }

    void on_snapshot_request_failure() noexcept
    {
        inc(m_snapshot_request_failures_total);
    }

    void on_decode_error() noexcept
    {
        inc(m_decode_errors_total);
    }

    void on_sequencer_gap_detected() noexcept
    {
        inc(m_sequencer_gap_detected_total);
    }

    void on_recovery() noexcept
    {
        inc(m_recoveries_total);
    }

    void on_queue_drop() noexcept
    {
        inc(m_queue_drops_total);
    }

    void on_orderbook_invalid() noexcept
    {
        inc(m_orderbook_invalid_total);
    }

    void set_queue_size(uint64_t size) noexcept
    {
        store(m_queue_size, size);
        update_max(m_queue_high_watermark, size);
    }

    void set_last_applied_update_id(uint64_t update_id) noexcept
    {
        store(m_last_applied_update_id, update_id);
        store(m_last_applied_update_unix_ms, now_unix_ms());
    }

    void set_recovering(bool value) noexcept
    {
        store(m_is_recovering, value);
    }

    Snapshot snapshot() const noexcept
    {
        return Snapshot{
            .ws_messages_total = load(m_ws_messages_total),
            .ws_reconnects_total = load(m_ws_reconnects_total),
            .snapshot_requests_total = load(m_snapshot_requests_total),
            .snapshot_request_failures_total = load(m_snapshot_request_failures_total),
            .decode_errors_total = load(m_decode_errors_total),
            .sequencer_gap_detected_total = load(m_sequencer_gap_detected_total),
            .recoveries_total = load(m_recoveries_total),
            .queue_drops_total = load(m_queue_drops_total),
            .orderbook_invalid_total = load(m_orderbook_invalid_total),
            .queue_size = load(m_queue_size),
            .queue_high_watermark = load(m_queue_high_watermark),
            .last_applied_update_id = load(m_last_applied_update_id),
            .is_recovering = load(m_is_recovering),
            .last_ws_message_unix_ms = load(m_last_ws_message_unix_ms),
            .last_snapshot_unix_ms = load(m_last_snapshot_unix_ms),
            .last_applied_update_unix_ms = load(m_last_applied_update_unix_ms),
        };
    }

private:
    counter_t m_ws_messages_total{0};
    counter_t m_ws_reconnects_total{0};
    counter_t m_snapshot_requests_total{0};
    counter_t m_snapshot_request_failures_total{0};
    counter_t m_decode_errors_total{0};
    counter_t m_sequencer_gap_detected_total{0};
    counter_t m_recoveries_total{0};
    counter_t m_queue_drops_total{0};
    counter_t m_orderbook_invalid_total{0};

    gauge_t m_queue_size{0};
    gauge_t m_queue_high_watermark{0};
    gauge_t m_last_applied_update_id{0};
    flag_t m_is_recovering{false};

    timestamp_t m_last_ws_message_unix_ms{0};
    timestamp_t m_last_snapshot_unix_ms{0};
    timestamp_t m_last_applied_update_unix_ms{0};
};

} // namespace metrics
