#ifndef GANYMEDE_COMMON_LOG_HH_
#define GANYMEDE_COMMON_LOG_HH_

#include <nlohmann/json.hpp>

namespace ganymede::common::log {

void error(nlohmann::json json);
void warn(nlohmann::json json);
void info(nlohmann::json json);

}

#endif