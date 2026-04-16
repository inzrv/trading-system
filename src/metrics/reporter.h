#pragma once

#include "format.h"
#include "registry.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace metrics
{

class Reporter
{
public:
    using sink_t = std::function<void(std::string)>;

    Reporter(const Registry& registry,
             std::chrono::milliseconds interval,
             sink_t sink)
        : m_registry(registry)
        , m_interval(interval)
        , m_sink(std::move(sink))
    {
    }

    ~Reporter()
    {
        stop();
    }

    void report_once() const
    {
        if (m_sink) {
            m_sink(format_snapshot(m_registry.snapshot()));
        }
    }

    void start()
    {
        std::lock_guard lock{m_mutex};
        if (m_running) {
            return;
        }

        m_running = true;
        m_thread = std::thread([this]() {
            run();
        });
    }

    void stop()
    {
        {
            std::lock_guard lock{m_mutex};
            if (!m_running) {
                return;
            }

            m_running = false;
        }

        m_cv.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    void run()
    {
        std::unique_lock lock{m_mutex};

        while (m_running) {
            if (m_cv.wait_for(lock, m_interval, [this] {
                    return !m_running;
                })) {
                break;
            }

            lock.unlock();
            report_once();

            lock.lock();
        }
    }

private:
    const Registry& m_registry;
    std::chrono::milliseconds m_interval;
    sink_t m_sink;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_running{false};
    std::thread m_thread;
};

} // namespace metrics
