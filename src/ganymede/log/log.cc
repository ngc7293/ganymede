#include <chrono>
#include <iostream>

#include <ctime>

#include "log.hh"

namespace ganymede::log {

Log& Log::get()
{
    static Log log;
    return log;
}

void Log::Output(Level level, const std::string& message, const nlohmann::json& tags) const
{
    for (const auto& sink: sinks) {
        if (sink) {
            sink->Output(level, message, tags);
        }
    }
}

void Log::AddSink(std::unique_ptr<LogSink>&& sink)
{
    sinks.emplace_back(std::move(sink));
}

void StdIoLogSink::Output(Level level, const std::string& message, const nlohmann::json& json) const
{
    const static char* LOG_LEVEL_STR[] = { "INVALID", "INFO", "WARNING", "ERROR" };

    const time_t now = time(NULL);
    const struct tm* utcnow = std::gmtime(&now);
    char rfc3339[24] = { 0 };
    std::strftime(rfc3339, sizeof(rfc3339), "%F %TZ", utcnow);

    auto& stream = (level <= Level::INFO ? std::cout : std::cerr);
    stream << "[" << rfc3339 << "] " << LOG_LEVEL_STR[static_cast<std::size_t>(level)] << ": " << message;
    
    if (not json.empty()) {
        stream << " " << json.dump();
    }
    
    stream << std::endl;
}

void error(const std::string& message, const nlohmann::json& json)
{
    Log::get().Output(Level::ERROR, message, json);
}

void warn(const std::string& message, const nlohmann::json& json)
{
    Log::get().Output(Level::WARNING, message, json);
}

void info(const std::string& message, const nlohmann::json& json)
{
    Log::get().Output(Level::INFO, message, json);
}

} // namespace ganymede::log