#pragma once

#include "common/queue.h"

#include <boost/lockfree/spsc_queue.hpp>

namespace binance
{

template<size_t Capacity>
class Queue : public IQueue
{
public:
    bool try_push(InputEnvelope&& item) override
    {
        return m_queue.push(item);
    }

    std::optional<InputEnvelope> try_pop() override
    {
        InputEnvelope item;
        if (!m_queue.pop(item)) {
            return std::nullopt;
        }

        return item;
    }

private:
    boost::lockfree::spsc_queue<
        InputEnvelope,
        boost::lockfree::capacity<Capacity>
    > m_queue;
};

} // namespace binance
