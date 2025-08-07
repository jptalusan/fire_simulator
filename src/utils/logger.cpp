#include "utils/logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace utils {

std::shared_ptr<spdlog::logger> Logger::s_logger = nullptr;

void Logger::init(const std::string& name) {
    std::string log_file = EnvLoader::getInstance()->get("LOGS_PATH", "../logs/output.log");
    std::vector<spdlog::sink_ptr> sinks;
    
    // Console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::err);
    // console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");
    console_sink->set_pattern("[%^%l%$] %v");
    sinks.push_back(console_sink);
    
    // File sink (if specified)
    if (!log_file.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");
        sinks.push_back(file_sink);
    }
    
    // Create logger
    s_logger = std::make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
    s_logger->set_level(spdlog::level::trace);
    s_logger->flush_on(spdlog::level::info);
    
    // Register as default logger
    spdlog::register_logger(s_logger);
    spdlog::set_default_logger(s_logger);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!s_logger) {
        init(); // Initialize with defaults if not already done
    }
    return s_logger;
}

void Logger::setLevel(const std::string& level) {
    auto logger = get();
    if (!logger) return;
    
    if (level == "trace") {
        logger->set_level(spdlog::level::trace);
    } else if (level == "debug") {
        logger->set_level(spdlog::level::debug);
    } else if (level == "info") {
        logger->set_level(spdlog::level::info);
    } else if (level == "warn") {
        logger->set_level(spdlog::level::warn);
    } else if (level == "error") {
        logger->set_level(spdlog::level::err);
    } else if (level == "critical") {
        logger->set_level(spdlog::level::critical);
    }
}


} // namespace utils
