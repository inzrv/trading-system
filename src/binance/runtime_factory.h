#pragma once

#include "config.h"
#include "common/runtime_factory.h"

namespace binance
{

class RuntimeFactory final : public IRuntimeFactory
{
public:
    explicit RuntimeFactory(Config config);

    RuntimeComponents create(boost::asio::io_context& io_ctx,
                             boost::asio::ssl::context& ssl_ctx) override;

private:
    Config m_config;
};

} // namespace binance
