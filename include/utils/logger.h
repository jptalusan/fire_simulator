#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>
#include "config/EnvLoader.h"
namespace utils {

/**
 * @brief Logger wrapper class using spdlog
 */
class Logger {
public:
    /**
     * @brief Initialize the logger
     * @param name Logger name
     * @param log_file Optional log file path
     */
    static void init(const std::string& name = "output.log");

    /**
     * @brief Get the logger instance
     * @return Shared pointer to logger
     */
    static std::shared_ptr<spdlog::logger> get();
    
    /**
     * @brief Set log level
     * @param level Log level (trace, debug, info, warn, error, critical)
     */
    static void setLevel(const std::string& level);

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

// Convenience macros
#define LOG_TRACE(...)    utils::Logger::get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    utils::Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...)     utils::Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...)     utils::Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    utils::Logger::get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) utils::Logger::get()->critical(__VA_ARGS__)

} // namespace utils
