#ifndef GANYMEDE__LOG__LOG_HH_
#define GANYMEDE__LOG__LOG_HH_

#include <nlohmann/json.hpp>

namespace ganymede::log {

void error(nlohmann::json json);
void warn(nlohmann::json json);
void info(nlohmann::json json);

} // namespace ganymede::log

#endif