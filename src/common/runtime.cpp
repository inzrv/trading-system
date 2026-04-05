#include "runtime.h"

#include "common/log.h"

#include <chrono>
#include <stdexcept>
#include <utility>

Runtime::Runtime(IRuntimeFactory& factory)
    : m_io_ctx()
    , m_ssl_ctx(ssl::context::tls_client)
    , m_work_guard(net::make_work_guard(m_io_ctx))
{
    m_ssl_ctx.set_default_verify_paths();
    m_ssl_ctx.set_verify_mode(ssl::verify_peer);

    auto components = factory.create(m_io_ctx, m_ssl_ctx);
    m_queue = std::move(components.queue);
    m_gateway = std::move(components.gateway);
    m_decoder = std::move(components.decoder);
    m_sequencer = std::move(components.sequencer);
    m_orderbook = std::move(components.orderbook);
    m_recovery_manager = std::move(components.recovery_manager);

    if (!m_queue || !m_gateway || !m_decoder || !m_sequencer || !m_orderbook || !m_recovery_manager) {
        throw std::invalid_argument("runtime factory returned incomplete components");
    }

    log::info("Runtime", "Runtime initialized with all components");
}

Runtime::~Runtime()
{
    stop();
}

void Runtime::run()
{
    log::info("Runtime", "starting...");
    m_running = true;

    m_io_thread = std::thread([this]() {
        m_io_ctx.run();
    });

    const auto init_res = m_recovery_manager->begin_initialize();
    if (!init_res) {
        log::error("Runtime", "initialization failed: {}", static_cast<int>(init_res.error()));
        stop();
        return;
    }

    run_core_loop();
}

void Runtime::stop()
{
    const bool was_running = m_running.exchange(false);
    if (!was_running) {
        return;
    }

    log::info("Runtime", "stopping...");
    m_recovery_manager->stop();
    m_work_guard.reset();
    m_io_ctx.stop();

    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
}

void Runtime::run_core_loop()
{
    while (m_running) {
        auto item = m_queue->try_pop();
        if (!item) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            continue;
        }

        const auto update_res = m_decoder->decode_diff(item->payload);
        if (!update_res) {
            log::warn("Runtime", "failed to parse update: {}", static_cast<int>(update_res.error()));
            m_recovery_manager->on_decode_error(update_res.error());
            continue;
        }

        auto update = update_res.value();

        if (m_recovery_manager->is_recovering()) {
            m_recovery_manager->buffer_update(std::move(update));
            const auto recover_res = m_recovery_manager->try_recover();
            if (!recover_res) {
                log::error("Runtime", "recovery failed during runtime loop: {}", static_cast<int>(recover_res.error()));
                stop();
                return;
            }
            continue;
        }

        const auto seq_res = m_sequencer->check(update);
        if (!seq_res) {
            const auto seq_error = seq_res.error();
            log::warn("Runtime", "sequencer error: {}", static_cast<int>(seq_error));

            if (seq_error == SequencingError::GAP_DETECTED) {
                const auto init_res = m_recovery_manager->begin_initialize();
                if (!init_res) {
                    log::error("Runtime", "initialization failed: {}", static_cast<int>(init_res.error()));
                    stop();
                    return;
                }
            }

            continue;
        }

        m_orderbook->apply(update);
        log::debug("Runtime", "applied update last_update={}", update.last_update);
    }
}
