#include "binance/config.h"
#include "binance/runtime_factory.h"
#include "common/log.h"
#include "common/runtime.h"
#include "common/config.h"
#include "common/signal_listener.h"

#include <fstream>
#include <iterator>
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    trading_system::log::initialize();
    log::info("Main", "trading system starting...");

    if (argc < 2) {
        log::error("Main", "usage: trading_system <config_path>");
        return 1;
    }

    const std::string config_path = argv[1];
    log::debug("Main", "config path: {}", config_path);

    std::ifstream config_file(config_path, std::ios::binary);
    if (!config_file) {
        log::error("Main", "failed to open config file: {}", config_path);
        return 1;
    }

    const std::string config_payload{
        std::istreambuf_iterator<char>{config_file},
        std::istreambuf_iterator<char>{}
    };

    const auto market = detect_market_from_config(config_payload);
    if (!market) {
        log::error("Main", "failed to parse config file market: {}", config_path);
        return 1;
    }

    log::info("Main", "detected market: {}", market_to_string(*market));

    try {
        std::unique_ptr<IRuntimeFactory> factory;

        if (*market == Market::BINANCE) {
            binance::Config config;
            if (!config.from_string(config_payload)) {
                log::error("Main", "failed to parse config file: {}", config_path);
                return 1;
            }

            log::debug("Main", "creating Binance runtime factory");
            factory = std::make_unique<binance::RuntimeFactory>(std::move(config));
        } else {
            log::error("Main", "unsupported market: {}", market_to_string(*market));
            return 1;
        }

        log::info("Main", "runtime factory created, starting runtime");

        Runtime runtime{*factory};

        SignalListener signal_listener([&runtime](int signal) {
            log::info("Main", "received signal {}, requesting shutdown", signal);
            runtime.stop();
        });

        if (!signal_listener.start()) {
            log::error("Main", "failed to start signal listener");
            return 1;
        }

        const auto run_res = runtime.run();
        signal_listener.stop();

        if (!run_res) {
            log::error("Main", "trading system finished with runtime error: {}", error_to_string(run_res.error()));
            return 1;
        }

        log::info("Main", "trading system finished successfully");
        return 0;
    } catch (const std::exception& e) {
        log::error("Main", "fatal error: {}", e.what());
        return 1;
    }
}
