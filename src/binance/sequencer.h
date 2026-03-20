#pragma once

#include "common/sequencer.h"

namespace binance
{

class Sequencer : public ISequencer
{
public:
    std::expected<void, SequencingError> check(const SequencedBookUpdate& update) override;
    void initialize(uint64_t last_update_id) override;
    void reset() override;

private:
    bool m_initialized{false};
    uint64_t m_last_update_id{0};
};

} // namespace binance
