#pragma once

#include "book_update.h"
#include "gateway.h"
#include "sequencer.h"
#include "orderbook.h"
#include "errors.h"

class IRecoveryManager
{
public:
    enum class State
    {
        STOPPED,
        RECOVERING,
        READY
    };

    virtual ~IRecoveryManager() = default;
    virtual std::expected<void, RecoveringError> begin_initialize() = 0;
    virtual std::expected<void, RecoveringError> try_recover() = 0;
    virtual void stop() = 0;
    virtual bool is_recovering() const = 0;
    virtual void buffer_update(SequencedBookUpdate update) = 0;
    virtual void on_decode_error(DecodingError error) const = 0;
    virtual void on_sequence_error(SequencingError error, SequencedBookUpdate update) const = 0;

    [[nodiscard]] State state() const noexcept
    {
        return m_state;
    }

protected:
    State m_state{State::STOPPED};
};
