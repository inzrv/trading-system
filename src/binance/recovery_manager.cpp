#include "recovery_manager.h"

#include "common/log.h"
#include "metrics/scoped_latency.h"

namespace binance
{

RecoveryManager::RecoveryManager(IGateway& gateway,
                                 IDecoder& decoder,
                                 ISequencer& sequencer,
                                 IOrderbook& orderbook,
                                 metrics::Registry& metrics)
    : m_gateway(gateway)
    , m_decoder(decoder)
    , m_sequencer(sequencer)
    , m_orderbook(orderbook)
    , m_metrics(metrics)
{}

std::expected<void, RecoveringError> RecoveryManager::begin_initialize()
{
    log::info("RecoveryManager", "begin initialize");
    m_state = State::RECOVERING;
    m_metrics.set_recovering(true);

    m_buffer.clear();
    m_snapshot.reset();
    m_snapshot_requested = false;

    m_orderbook.reset();
    m_sequencer.reset();

    m_gateway.start();

    const auto wait_res = m_gateway.wait_until_running(kGatewayStartTimeout);
    if (!wait_res) {
        log::error("RecoveryManager", "failed to start gateway: {}", error_to_string(wait_res.error()));
        m_state = State::STOPPED;
        m_metrics.set_recovering(false);
        return std::unexpected(RecoveringError::GATEWAY_START_ERROR);
    }

    log::info("RecoveryManager", "gateway started");
    return {};
}

std::expected<void, RecoveringError> RecoveryManager::try_recover()
{
    if (m_state != State::RECOVERING) {
        return {};
    }

    if (!m_snapshot_requested) {
        log::info("RecoveryManager", "requesting snapshot...");
        const auto snapshot_res = m_gateway.request_snapshot();
        if (!snapshot_res) {
            log::error("RecoveryManager", "snapshot request failed during recovery");
            return std::unexpected(RecoveringError::SNAPSHOT_REQUEST_ERROR);
        }

        const auto decode_res = metrics::measure(m_metrics, &metrics::Registry::observe_snapshot_decode, [&] {
            return m_decoder.decode_snapshot(*snapshot_res);
        });
        if (!decode_res) {
            log::error("RecoveryManager", "snapshot parsing failed during recovery");
            m_metrics.on_decode_error();
            return std::unexpected(RecoveringError::SNAPSHOT_PARSING_ERROR);
        }

        m_snapshot = *decode_res;
        m_snapshot_requested = true;
    }

    const auto first_pos = find_first_applicable_update(m_snapshot->last_update_id);
    if (!first_pos) {
        log::debug("RecoveryManager", "no applicable update found yet during recovery");
        return {};
    }

    log::info("RecoveryManager", "applying buffered updates from pos {}", *first_pos);
    m_orderbook.reset();
    m_orderbook.initialize(*m_snapshot);
    m_sequencer.reset();
    m_sequencer.initialize(m_snapshot->last_update_id);

    const auto applying_status = metrics::measure(m_metrics, &metrics::Registry::observe_recovery_replay, [&] {
        return apply_updates_from(*first_pos);
    });
    if (applying_status == ApplyResult::RESTART_REQUIRED) {
        log::warn("RecoveryManager", "buffered replay detected a gap, restarting recovery");
        const auto init_res = begin_initialize();
        if (!init_res) {
            log::error("RecoveryManager", "re-initialization failed after buffered replay gap: {}",
                       error_to_string(init_res.error()));
            return std::unexpected(init_res.error());
        }
        return {};
    }

    m_buffer.clear();
    m_snapshot.reset();
    m_snapshot_requested = false;
    m_state = State::READY;
    m_metrics.on_recovery();
    m_metrics.set_recovering(false);
    log::info("RecoveryManager", "recovered and ready");
    return {};
}

void RecoveryManager::stop()
{
    log::info("RecoveryManager", "stopping...");
    m_gateway.stop();
    m_buffer.clear();
    m_snapshot.reset();
    m_snapshot_requested = false;
    m_sequencer.reset();
    m_orderbook.reset();
    m_state = State::STOPPED;
    m_metrics.set_recovering(false);
}

bool RecoveryManager::is_recovering() const
{
    return m_state == State::RECOVERING;
}

void RecoveryManager::on_decode_error(DecodingError error) const
{
    log::warn("RecoveryManager", "decode error: {}", error_to_string(error));
    m_metrics.on_decode_error();
}

void RecoveryManager::buffer_update(SequencedBookUpdate update)
{
    m_buffer.push_back(std::move(update));
}

std::optional<size_t> RecoveryManager::find_first_applicable_update(uint64_t last_update_id) const
{
    const auto next_update_id = last_update_id + 1;

    for (size_t i = 0; i < m_buffer.size(); ++i) {
        const auto& update = m_buffer[i];

        if (update.last_update <= last_update_id) {
            continue;
        }

        if (update.first_update <= next_update_id && next_update_id <= update.last_update) {
            return i;
        }
    }

    return std::nullopt;
}

RecoveryManager::ApplyResult RecoveryManager::apply_updates_from(size_t pos)
{
    for (size_t i = pos; i < m_buffer.size(); ++i) {
        const auto& update = m_buffer[i];
        const auto seq_res = metrics::measure(m_metrics, &metrics::Registry::observe_sequencer, [&] {
            return m_sequencer.check(update);
        });
        if (!seq_res) {
            if (seq_res.error() == SequencingError::GAP_DETECTED) {
                m_metrics.on_sequencer_gap_detected();
            }
            return ApplyResult::RESTART_REQUIRED;
        }

        metrics::measure(m_metrics, &metrics::Registry::observe_orderbook_apply, [&] {
            m_orderbook.apply(update);
        });
        m_metrics.set_last_applied_update_id(update.last_update);
    }

    return ApplyResult::APPLIED;
}

} // namespace binance
