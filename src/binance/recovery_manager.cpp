#include "recovery_manager.h"

#include <thread>

namespace binance
{

RecoveryManager::RecoveryManager(IGateway& gateway,
                                 IDecoder& decoder,
                                 ISequencer& sequencer,
                                 IOrderbook& orderbook)
    : m_gateway(gateway)
    , m_decoder(decoder)
    , m_sequencer(sequencer)
    , m_orderbook(orderbook)
{}

std::expected<void, RecoveringError> RecoveryManager::begin_initialize()
{
    m_state = State::RECOVERING;

    m_buffer.clear();
    m_snapshot.reset();
    m_snapshot_requested = false;

    m_orderbook.reset();
    m_sequencer.reset();

    m_gateway.start();

    const auto wait_res = m_gateway.wait_until_running(kGatewayStartTimeout);
    if (!wait_res) {
        m_state = State::STOPPED;
        return std::unexpected(RecoveringError::GATEWAY_START_ERROR);
    }

    return {};
}

std::expected<void, RecoveringError> RecoveryManager::try_recover()
{
    if (m_state != State::RECOVERING) {
        return {};
    }

    if (!m_snapshot_requested) {
        const auto snapshot_res = m_gateway.request_snapshot();
        if (!snapshot_res) {
            return std::unexpected(RecoveringError::SNAPSHOT_REQUEST_ERROR);
        }

        const auto decode_res = m_decoder.decode_snapshot(*snapshot_res);
        if (!decode_res) {
            return std::unexpected(RecoveringError::SNAPSHOT_PARSING_ERROR);
        }

        m_snapshot = *decode_res;
        m_snapshot_requested = true;
    }

    if (!m_snapshot.has_value()) {
        return {};
    }

    const auto first_pos = find_first_applicable_update(m_snapshot->last_update_id);
    if (!first_pos) {
        return {};
    }

    m_orderbook.reset();
    m_orderbook.initialize(*m_snapshot);
    m_sequencer.reset();
    m_sequencer.initialize(m_snapshot->last_update_id);

    const auto applying_status = apply_updates_from(*first_pos);
    if (!applying_status) {
        return {};
    }

    m_buffer.clear();
    m_snapshot.reset();
    m_snapshot_requested = false;
    m_state = State::READY;
    return {};
}

void RecoveryManager::stop()
{
    m_gateway.stop();
    m_buffer.clear();
    m_snapshot.reset();
    m_snapshot_requested = false;
    m_sequencer.reset();
    m_orderbook.reset();
    m_state = State::STOPPED;
}

bool RecoveryManager::is_recovering() const
{
    return m_state == State::RECOVERING;
}

void RecoveryManager::on_decode_error(DecodingError) const
{}

void RecoveryManager::buffer_update(SequencedBookUpdate update)
{
    m_buffer.push_back(std::move(update));
}

void RecoveryManager::on_sequence_error(SequencingError, SequencedBookUpdate) const
{}

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

bool RecoveryManager::apply_updates_from(size_t pos)
{
    for (size_t i = pos; i < m_buffer.size(); ++i) {
        const auto& update = m_buffer[i];
        if (!m_sequencer.check(update)) {
            begin_initialize();
            return false;
        }

        m_orderbook.apply(update);
    }

    return true;
}

} // namespace binance
