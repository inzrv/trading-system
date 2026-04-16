#pragma once

#include <atomic>
#include <csignal>
#include <functional>
#include <pthread.h>
#include <thread>

class SignalListener
{
public:
    using handler_t = std::function<void(int)>;

    explicit SignalListener(handler_t handler);
    ~SignalListener();

    bool start();
    void stop();

private:
    bool initialize_signal_set();
    bool block_signals();
    void restore_signal_mask();
    void run();

private:
    handler_t m_handler;
    sigset_t m_signal_set{};
    sigset_t m_old_mask{};
    std::atomic<bool> m_stop_requested{false};
    std::thread m_thread;
    bool m_started{false};
    bool m_signal_set_initialized{false};
    bool m_mask_installed{false};
};
