#include "sequencer.h"

namespace binance
{

std::expected<void, SequencingError> Sequencer::check(const SequencedBookUpdate& update)
{
    if (!m_initialized) {
        return std::unexpected(SequencingError::NOT_INITIALIZED);
    }

    const auto expected_next = m_last_update_id + 1;

    if (update.last_update < expected_next) {
        return {};
    }

    if (update.first_update > expected_next) {
        return std::unexpected(SequencingError::GAP_DETECTED);
    }

    m_last_update_id = update.last_update;
    return {};
}

void Sequencer::initialize(uint64_t last_update_id)
{
    m_last_update_id = last_update_id;
    m_initialized = true;
}

void Sequencer::reset()
{
    m_last_update_id = 0;
    m_initialized = false;
}

} // namespace binance
