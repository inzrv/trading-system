#pragma once

#include "envelope.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>

class IQueue
{
public:
    virtual ~IQueue() = default;
    virtual bool try_push(InputEnvelope&& item) = 0;
    virtual InputEnvelope wait_pop() = 0;
};

template<size_t Capacity>
class Queue final : public IQueue
{
public:
    bool try_push(InputEnvelope&& item) override
    {
        {
            std::lock_guard lock{m_mutex};
            if (m_queue.size() >= Capacity) {
                return false;
            }

            m_queue.push_back(std::move(item));
        }

        m_cv.notify_one();
        return true;
    }

    InputEnvelope wait_pop() override
    {
        std::unique_lock lock{m_mutex};
        m_cv.wait(lock, [this] {
            return !m_queue.empty();
        });

        InputEnvelope item = std::move(m_queue.front());
        m_queue.pop_front();
        return item;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<InputEnvelope> m_queue;
};
