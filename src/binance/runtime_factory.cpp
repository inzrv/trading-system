#include "runtime_factory.h"

#include "decoder.h"
#include "gateway.h"
#include "orderbook.h"
#include "queue.h"
#include "recovery_manager.h"
#include "sequencer.h"

#include <utility>

namespace binance
{

RuntimeFactory::RuntimeFactory(Config config)
    : m_config(std::move(config))
{}

RuntimeComponents RuntimeFactory::create(boost::asio::io_context& io_ctx,
                                         boost::asio::ssl::context& ssl_ctx)
{
    RuntimeComponents components;
    components.queue = std::make_shared<Queue<10'000>>();
    components.gateway = std::make_unique<Gateway>(m_config, io_ctx, ssl_ctx, components.queue);
    components.decoder = std::make_unique<Decoder>();
    components.sequencer = std::make_unique<Sequencer>();
    components.orderbook = std::make_unique<Orderbook>();
    components.recovery_manager =
        std::make_unique<RecoveryManager>(*components.gateway, *components.decoder, *components.sequencer, *components.orderbook);

    return components;
}

} // namespace binance
