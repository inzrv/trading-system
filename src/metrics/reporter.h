#pragma once

#include "common/worker.h"
#include "format.h"
#include "registry.h"

#include <chrono>
#include <functional>
#include <string>
#include <utility>

namespace metrics
{

class Reporter final : public Worker
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

private:
    void run() override
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
};

} // namespace metrics
