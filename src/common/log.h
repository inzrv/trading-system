#pragma once

#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace trading_system::log
{
inline void initialize()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("trading_system", console_sink);
    spdlog::set_default_logger(std::move(logger));
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::info);
}

inline std::string add_module_prefix(std::string_view module, std::string_view fmt)
{
    return "[" + std::string(module) + "]: " + std::string(fmt);
}

template <typename... Args>
inline void trace(std::string_view module, std::string_view fmt, Args&&... args)
{
    auto full_fmt = add_module_prefix(module, fmt);
    spdlog::log(spdlog::level::trace, fmt::runtime(full_fmt), std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(std::string_view module, std::string_view fmt, Args&&... args)
{
    auto full_fmt = add_module_prefix(module, fmt);
    spdlog::log(spdlog::level::debug, fmt::runtime(full_fmt), std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(std::string_view module, std::string_view fmt, Args&&... args)
{
    auto full_fmt = add_module_prefix(module, fmt);
    spdlog::log(spdlog::level::info, fmt::runtime(full_fmt), std::forward<Args>(args)...);
}

template <typename... Args>
inline void warn(std::string_view module, std::string_view fmt, Args&&... args)
{
    auto full_fmt = add_module_prefix(module, fmt);
    spdlog::log(spdlog::level::warn, fmt::runtime(full_fmt), std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(std::string_view module, std::string_view fmt, Args&&... args)
{
    auto full_fmt = add_module_prefix(module, fmt);
    spdlog::log(spdlog::level::err, fmt::runtime(full_fmt), std::forward<Args>(args)...);
}

template <typename... Args>
inline void critical(std::string_view module, std::string_view fmt, Args&&... args)
{
    auto full_fmt = add_module_prefix(module, fmt);
    spdlog::log(spdlog::level::critical, fmt::runtime(full_fmt), std::forward<Args>(args)...);
}

inline void set_level(spdlog::level::level_enum level)
{
    spdlog::set_level(level);
}
} // namespace trading_system::log

namespace log = trading_system::log;

