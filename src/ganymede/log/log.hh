#ifndef GANYMEDE__LOG__LOG_HH_
#define GANYMEDE__LOG__LOG_HH_

#include <nlohmann/json.hpp>

namespace ganymede::log {

enum class Level {
    INVALID = 0,
    INFO    = 1,
    WARNING = 2,
    ERROR   = 3
};

class LogSink {
public:
    virtual void Output(Level level, const std::string& message, const nlohmann::json& tags) const = 0;
};

class Log : public LogSink {
public:
    static Log& get();

    void Output(Level level, const std::string& message, const nlohmann::json& tags) const override;

    void AddSink(std::unique_ptr<LogSink>&& sink);

private:
    Log() = default;

private:
    std::vector<std::unique_ptr<LogSink>> sinks;
};

class StdIoLogSink : public LogSink {
public:
    void Output(Level level, const std::string& message, const nlohmann::json& tags) const override;
};

void error(const std::string& message, const nlohmann::json& json = {});
void warn(const std::string& message, const nlohmann::json& json = {});
void info(const std::string& message, const nlohmann::json& json = {});

} // namespace ganymede::log

#endif