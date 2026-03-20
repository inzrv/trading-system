#include "runtime.h"
#include "binance/gateway.h"
#include "binance/queue.h"
#include "binance/decoder.h"
#include "binance/sequencer.h"
#include "binance/recovery_manager.h"
#include "binance/orderbook.h"

#include <iostream>
#include <chrono>

Runtime::Runtime()
    : m_io_ctx()
    , m_ssl_ctx(ssl::context::tls_client)
    , m_work_guard(net::make_work_guard(m_io_ctx))
    , m_queue(std::make_shared<binance::Queue<10'000>>())
    , m_gateway(std::make_unique<binance::Gateway>(m_io_ctx, m_ssl_ctx, m_queue))
    , m_decoder(std::make_unique<binance::Decoder>())
    , m_sequencer(std::make_unique<binance::Sequencer>())
    , m_orderbook(std::make_unique<binance::Orderbook>())
    , m_recovery_manager(std::make_unique<binance::RecoveryManager>(*m_gateway, *m_decoder, *m_sequencer, *m_orderbook))
{
    m_ssl_ctx.set_default_verify_paths();
    m_ssl_ctx.set_verify_mode(ssl::verify_peer);
}

Runtime::~Runtime()
{
    stop();
}

void Runtime::run()
{
    m_running = true;

    m_io_thread = std::thread([this]() {
        m_io_ctx.run();
    });

    const auto init_res = m_recovery_manager->begin_initialize();
    if (!init_res) {
        [[maybe_unused]] const auto _ = init_res.error();
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
            m_recovery_manager->on_decode_error(update_res.error());
            continue;
        }

        auto update = update_res.value();

        if (m_recovery_manager->is_recovering()) {
            m_recovery_manager->buffer_update(std::move(update));
            const auto recover_res = m_recovery_manager->try_recover();
            if (!recover_res) {
                stop();
                return;
            }
            continue;
        }

        const auto seq_res = m_sequencer->check(update);
        if (!seq_res) {
            m_recovery_manager->on_sequence_error(seq_res.error(), std::move(update));
            continue;
        }

        m_orderbook->apply(update);
    }
}
