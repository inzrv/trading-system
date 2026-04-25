#pragma once

#include "common/time.h"
#include "registry.h"

#include <utility>

namespace metrics
{

class ScopedLatency
{
public:
    using observe_fn_t = void (Registry::*)(std::chrono::nanoseconds) noexcept;

    ScopedLatency(Registry& registry, observe_fn_t observe_fn) noexcept
        : m_registry(registry)
        , m_observe_fn(observe_fn)
        , m_started_at(latency_clock::now())
    {
    }

    ScopedLatency(const ScopedLatency&) = delete;
    ScopedLatency& operator=(const ScopedLatency&) = delete;
    ScopedLatency(ScopedLatency&&) = delete;
    ScopedLatency& operator=(ScopedLatency&&) = delete;

    ~ScopedLatency() noexcept
    {
        (m_registry.*m_observe_fn)(elapsed_since(m_started_at));
    }

private:
    Registry& m_registry;
    observe_fn_t m_observe_fn;
    latency_clock::time_point m_started_at;
};

template<typename Fn>
decltype(auto) measure(Registry& registry, ScopedLatency::observe_fn_t observe_fn, Fn&& fn)
{
    const ScopedLatency timer{registry, observe_fn};
    return std::forward<Fn>(fn)();
}

} // namespace metrics
