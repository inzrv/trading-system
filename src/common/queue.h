#pragma once

#include "envelope.h"
#include "metrics/registry.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>

class IQueue
{
public:
    virtual ~IQueue() = default;
    virtual bool try_push(InputEnvelope&& item) = 0;
    virtual std::optional<InputEnvelope> wait_pop() = 0;
    virtual void close() = 0;
};

template<size_t Capacity>
class Queue final : public IQueue
{
public:
    explicit Queue(metrics::Registry& metrics) noexcept
        : m_metrics(metrics)
    {
    }

    bool try_push(InputEnvelope&& item) override
    {
        {
            std::lock_guard lock{m_mutex};

            if (m_closed) {
                return false;
            }

            if (m_queue.size() >= Capacity) {
                m_metrics.on_queue_drop();
                return false;
            }

            m_queue.push_back(std::move(item));
            m_metrics.set_queue_size(m_queue.size());
        }

        m_cv.notify_one();
        return true;
    }

    std::optional<InputEnvelope> wait_pop() override
    {
        std::unique_lock lock{m_mutex};
        m_cv.wait(lock, [this] {
            return !m_queue.empty() || m_closed;
        });

        if (m_queue.empty()) {
            return std::nullopt;
        }

        InputEnvelope item = std::move(m_queue.front());
        m_queue.pop_front();
        m_metrics.set_queue_size(m_queue.size());
        return item;
    }

    void close() override
    {
        {
            std::lock_guard lock{m_mutex};
            m_closed = true;
        }

        m_cv.notify_all();
    }

private:
    metrics::Registry& m_metrics;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<InputEnvelope> m_queue;
    bool m_closed{false};
};
