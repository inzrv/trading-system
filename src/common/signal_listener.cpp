#include "signal_listener.h"

#include "log.h"

#include <cerrno>
#include <utility>

namespace
{

const char* signal_to_string(int signal) noexcept
{
    switch (signal) {
        case SIGINT: return "SIGINT";
        case SIGTERM: return "SIGTERM";
        default: return "UNKNOWN";
    }
}

} // namespace

SignalListener::SignalListener(handler_t handler)
    : m_handler(std::move(handler))
{
}

SignalListener::~SignalListener()
{
    stop();
}

bool SignalListener::start()
{
    if (m_started) {
        return true;
    }

    if (!initialize_signal_set()) {
        log::error("SignalListener", "failed to initialize shutdown signal set");
        return false;
    }

    if (!block_signals()) {
        log::error("SignalListener", "failed to block shutdown signals");
        return false;
    }

    m_stop_requested.store(false);
    m_thread = std::thread([this]() {
        run();
    });
    m_started = true;
    return true;
}

void SignalListener::stop()
{
    if (!m_started) {
        restore_signal_mask();
        return;
    }

    m_stop_requested.store(true);

    const int wake_result = pthread_kill(m_thread.native_handle(), SIGTERM);
    if (wake_result != 0 && wake_result != ESRCH) {
        log::warn("SignalListener", "failed to wake listener thread: {}", wake_result);
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_started = false;
    restore_signal_mask();
}

bool SignalListener::initialize_signal_set()
{
    if (m_signal_set_initialized) {
        return true;
    }

    if (sigemptyset(&m_signal_set) != 0) {
        return false;
    }

    if (sigaddset(&m_signal_set, SIGINT) != 0) {
        return false;
    }

    if (sigaddset(&m_signal_set, SIGTERM) != 0) {
        return false;
    }

    m_signal_set_initialized = true;
    return true;
}

bool SignalListener::block_signals()
{
    if (m_mask_installed) {
        return true;
    }

    if (pthread_sigmask(SIG_BLOCK, &m_signal_set, &m_old_mask) != 0) {
        return false;
    }

    m_mask_installed = true;
    return true;
}

void SignalListener::restore_signal_mask()
{
    if (!m_mask_installed) {
        return;
    }

    pthread_sigmask(SIG_SETMASK, &m_old_mask, nullptr);
    m_mask_installed = false;
}

void SignalListener::run()
{
    int signal = 0;
    const int wait_result = sigwait(&m_signal_set, &signal);
    if (wait_result != 0) {
        log::error("SignalListener", "sigwait failed with error {}", wait_result);
        return;
    }

    if (m_stop_requested.load()) {
        return;
    }

    log::info("SignalListener", "received {}", signal_to_string(signal));
    if (m_handler) {
        m_handler(signal);
    }
}
