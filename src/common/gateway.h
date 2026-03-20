#pragma once

#include "errors.h"

#include <expected>
#include <string>
#include <chrono>

class IGateway
{
public:
    enum class State
    {
        STOPPED,
        STARTING,
        RUNNING,
        RECOVERING,
        STOPPING,
        FAILED
    };

    virtual ~IGateway() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void restart() = 0;
    virtual std::expected<std::string, GatewayError> request_snapshot() = 0;
    virtual std::expected<void, GatewayError> wait_until_running(std::chrono::milliseconds timeout) = 0;

    [[nodiscard]] State state() const noexcept
    {
        return m_state;
    }

protected:
    State m_state{State::STOPPED};
};
