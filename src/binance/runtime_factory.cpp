#include "runtime_factory.h"

#include "decoder.h"
#include "gateway.h"
#include "orderbook.h"
#include "recovery_manager.h"
#include "sequencer.h"
#include "common/log.h"
#include "common/queue.h"
#include "replay/market_data_recorder.h"
#include "replay/replay_gateway.h"

#include <stdexcept>
#include <utility>

namespace binance
{

RuntimeFactory::RuntimeFactory(Config config)
    : m_config(std::move(config))
{}

RuntimeComponents RuntimeFactory::create(boost::asio::io_context& io_ctx,
                                         boost::asio::ssl::context& ssl_ctx)
{
    log::info("RuntimeFactory", "creating components...");
    RuntimeComponents components;

    components.metrics = std::make_unique<metrics::Registry>();
    components.queue = std::make_shared<Queue<10'000>>(*components.metrics);

    if (m_config.mode == RuntimeMode::REPLAY) {
        log::info("RuntimeFactory",
                  "replay mode enabled: updates_path={} snapshots_path={}",
                  m_config.replay_conf->updates_path,
                  m_config.replay_conf->snapshots_path);

        auto market_data = replay::load_market_data(
            m_config.replay_conf->updates_path,
            m_config.replay_conf->snapshots_path);
        if (!market_data) {
            throw std::runtime_error("failed to load replay market data: " + market_data.error());
        }

        components.gateway = std::make_unique<replay::ReplayGateway>(
            components.queue,
            std::move(*market_data));
    } else if (m_config.recording_conf) {
        log::info("RuntimeFactory",
                  "market data recording enabled: updates_path={} snapshots_path={} flush_interval={}ms",
                  m_config.recording_conf->updates_path,
                  m_config.recording_conf->snapshots_path,
                  m_config.recording_conf->flush_interval.count());
        components.market_data_recorder = std::make_unique<replay::MarketDataRecorder>(
            m_config.recording_conf->updates_path,
            m_config.recording_conf->snapshots_path,
            m_config.recording_conf->flush_interval);
    }

    if (!components.gateway) {
        components.gateway = std::make_unique<Gateway>(
            m_config,
            io_ctx,
            ssl_ctx,
            components.queue,
            *components.metrics,
            components.market_data_recorder.get());
    }

    components.decoder = std::make_unique<Decoder>();
    components.sequencer = std::make_unique<Sequencer>();
    components.orderbook = std::make_unique<Orderbook>();
    components.recovery_manager =
        std::make_unique<RecoveryManager>(
            *components.gateway,
            *components.decoder,
            *components.sequencer,
            *components.orderbook,
            *components.metrics);

    log::info("RuntimeFactory", "created all components");
    return components;
}

} // namespace binance
