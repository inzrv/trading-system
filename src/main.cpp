#include "common/runtime.h"
#include "common/config.h"
#include "binance/config.h"
#include "binance/runtime_factory.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "usage: trading_system <config_path>\n";
        return 1;
    }

    const std::string config_path = argv[1];
    std::ifstream config_file(config_path, std::ios::binary);
    if (!config_file) {
        std::cerr << "failed to open config file: " << config_path << '\n';
        return 1;
    }

    const std::string config_payload{
        std::istreambuf_iterator<char>{config_file},
        std::istreambuf_iterator<char>{}
    };

    const auto market = detect_market_from_config(config_payload);
    if (!market) {
        std::cerr << "failed to parse config file market: " << config_path << '\n';
        return 1;
    }

    try {
        std::unique_ptr<IRuntimeFactory> factory;

        if (*market == Market::BINANCE) {
            binance::Config config;
            if (!config.from_string(config_payload)) {
                std::cerr << "failed to parse config file: " << config_path << '\n';
                return 1;
            }

            factory = std::make_unique<binance::RuntimeFactory>(std::move(config));
        } else {
            std::cerr << "unsupported market: " << market_to_string(*market) << '\n';
            return 1;
        }

        Runtime runtime{*factory};
        runtime.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}
