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

class LatencyStats
{
public:
    void observe(std::chrono::nanoseconds duration) noexcept
    {
        const auto ns = duration.count() > 0
            ? static_cast<uint64_t>(duration.count())
            : 0ull;

        inc(m_count);
        m_total_ns.fetch_add(ns, std::memory_order_relaxed);
        update_max(m_max_ns, ns);
    }

    LatencySnapshot snapshot() const noexcept
    {
        return LatencySnapshot{
            .count = load(m_count),
            .total_ns = load(m_total_ns),
            .max_ns = load(m_max_ns),
        };
    }

private:
    counter_t m_count{0};
    gauge_t m_total_ns{0};
    gauge_t m_max_ns{0};
};

// Registry stores telemetry as independent relaxed atomics.
// Snapshots are intentionally approximate: related fields may be observed in
// slightly different states, which is acceptable for metrics and keeps the
// hot path cheap and lock-free.
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

    void observe_decode(std::chrono::nanoseconds duration) noexcept
    {
        m_decode_latency.observe(duration);
    }

    void observe_sequencer(std::chrono::nanoseconds duration) noexcept
    {
        m_sequencer_latency.observe(duration);
    }

    void observe_orderbook_apply(std::chrono::nanoseconds duration) noexcept
    {
        m_orderbook_apply_latency.observe(duration);
    }

    void observe_snapshot_request(std::chrono::nanoseconds duration) noexcept
    {
        m_snapshot_request_latency.observe(duration);
    }

    void observe_snapshot_decode(std::chrono::nanoseconds duration) noexcept
    {
        m_snapshot_decode_latency.observe(duration);
    }

    void observe_recovery_replay(std::chrono::nanoseconds duration) noexcept
    {
        m_recovery_replay_latency.observe(duration);
    }

    void observe_queue_wait(std::chrono::nanoseconds duration) noexcept
    {
        m_queue_wait_latency.observe(duration);
    }

    void observe_wait_pop(std::chrono::nanoseconds duration) noexcept
    {
        m_wait_pop_latency.observe(duration);
    }

    void observe_message_e2e(std::chrono::nanoseconds duration) noexcept
    {
        m_message_e2e_latency.observe(duration);
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
            .decode_latency = m_decode_latency.snapshot(),
            .sequencer_latency = m_sequencer_latency.snapshot(),
            .orderbook_apply_latency = m_orderbook_apply_latency.snapshot(),
            .snapshot_request_latency = m_snapshot_request_latency.snapshot(),
            .snapshot_decode_latency = m_snapshot_decode_latency.snapshot(),
            .recovery_replay_latency = m_recovery_replay_latency.snapshot(),
            .queue_wait_latency = m_queue_wait_latency.snapshot(),
            .wait_pop_latency = m_wait_pop_latency.snapshot(),
            .message_e2e_latency = m_message_e2e_latency.snapshot(),
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

    LatencyStats m_decode_latency;
    LatencyStats m_sequencer_latency;
    LatencyStats m_orderbook_apply_latency;
    LatencyStats m_snapshot_request_latency;
    LatencyStats m_snapshot_decode_latency;
    LatencyStats m_recovery_replay_latency;
    LatencyStats m_queue_wait_latency;
    LatencyStats m_wait_pop_latency;
    LatencyStats m_message_e2e_latency;
};

} // namespace metrics
