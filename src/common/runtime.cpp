#include "runtime.h"

#include "common/orderbook_validation.h"
#include "common/log.h"

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
    m_metrics = std::move(components.metrics);
    m_queue = std::move(components.queue);
    m_gateway = std::move(components.gateway);
    m_decoder = std::move(components.decoder);
    m_sequencer = std::move(components.sequencer);
    m_orderbook = std::move(components.orderbook);
    m_recovery_manager = std::move(components.recovery_manager);

    if (!m_metrics || !m_queue || !m_gateway || !m_decoder || !m_sequencer || !m_orderbook || !m_recovery_manager) {
        throw std::invalid_argument("runtime factory returned incomplete components");
    }

    m_metrics_reporter = std::make_unique<metrics::Reporter>(
        *m_metrics,
        kMetricsReportInterval,
        [](std::string payload) {
            log::info("Metrics", "{}", payload);
        });

    log::info("Runtime", "Runtime initialized with all components");
}

Runtime::~Runtime()
{
    stop();
}

std::expected<void, RuntimeError> Runtime::run()
{
    log::info("Runtime", "starting...");
    m_running = true;
    m_metrics_reporter->start();
    m_metrics_reporter->report_once();

    m_io_thread = std::thread([this]() {
        m_io_ctx.run();
    });

    const auto init_res = m_recovery_manager->begin_initialize();
    if (!init_res) {
        log::error("Runtime", "initialization failed: {}", error_to_string(init_res.error()));
        stop();
        return std::unexpected(RuntimeError::INITIALIZATION_ERROR);
    }

    return run_core_loop();
}

void Runtime::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }

    log::info("Runtime", "stopping...");
    m_queue->close();
    m_recovery_manager->stop();
    m_metrics_reporter->report_once();
    m_metrics_reporter->stop();
    m_work_guard.reset();
    m_io_ctx.stop();

    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
}

std::expected<void, RuntimeError> Runtime::run_core_loop()
{
    for (;;) {
        auto item = m_queue->wait_pop();
        if (!item) {
            log::info("Runtime", "input queue closed, stopping core loop");
            return {};
        }

        const auto update_res = m_decoder->decode_diff(item->payload);
        if (!update_res) {
            log::warn("Runtime", "failed to parse update: {}", error_to_string(update_res.error()));
            m_recovery_manager->on_decode_error(update_res.error());
            continue;
        }

        auto update = update_res.value();

        if (m_recovery_manager->is_recovering()) {
            m_recovery_manager->buffer_update(std::move(update));
            const auto recover_res = m_recovery_manager->try_recover();
            if (!recover_res) {
                log::error("Runtime", "recovery failed during runtime loop: {}", error_to_string(recover_res.error()));
                stop();
                return std::unexpected(RuntimeError::RECOVERY_ERROR);
            }
            continue;
        }

        const auto seq_res = m_sequencer->check(update);
        if (!seq_res) {
            const auto seq_error = seq_res.error();
            log::warn("Runtime", "sequencer error: {}", error_to_string(seq_error));

            if (seq_error == SequencingError::GAP_DETECTED) {
                m_metrics->on_sequencer_gap_detected();
                const auto init_res = m_recovery_manager->begin_initialize();
                if (!init_res) {
                    log::error("Runtime", "initialization failed: {}", error_to_string(init_res.error()));
                    stop();
                    return std::unexpected(RuntimeError::INITIALIZATION_ERROR);
                }
            }

            continue;
        }

        m_orderbook->apply(update);
        m_metrics->set_last_applied_update_id(update.last_update);
        log::debug("Runtime", "applied update last_update={}", update.last_update);

        const auto l1 = m_orderbook->l1();
        if (!is_valid(l1)) {
            m_metrics->on_orderbook_invalid();
            log::warn("Runtime", "orderbook is invalid: bid_price={}, ask_price={}", l1.best_bid->price, l1.best_ask->price);
        }
    }

    return {};
}
