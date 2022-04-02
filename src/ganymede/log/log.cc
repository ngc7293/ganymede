#include <chrono>
#include <iostream>

#include <ctime>

#include "log.hh"

namespace ganymede::log {

void output(nlohmann::json& json)
{
    const time_t now = time(NULL);
    const struct tm* utcnow = std::gmtime(&now);
    char rfc3339[24] = { 0 };

    std::strftime(rfc3339, sizeof(rfc3339), "%F %TZ", utcnow);
    json["times"] = rfc3339;
    std::cerr << json.dump() << std::endl;
}

void error(nlohmann::json json)
{
    json["severity"] = "ERROR";
    output(json);
}

void warn(nlohmann::json json)
{
    json["severity"] = "WARNING";
    output(json);
}

void info(nlohmann::json json)
{
    json["severity"] = "INFO";
    output(json);
}

} // namespace ganymede::log