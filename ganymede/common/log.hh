#ifndef GANYMEDE_COMMON_LOG_HH_
#define GANYMEDE_COMMON_LOG_HH_

#include <iostream>
#include <string>
#include <sstream>

namespace ganymede::common::log {

struct kvp {
    std::string key, value;
};

void OutputJsonKvpToStream(std::ostream& os, const std::initializer_list<const kvp>& list)
{
    for (const kvp& pair : list) {
        os << ",\"" << pair.key << "\":\"" << pair.value << "\"";
    }
}

void error(std::initializer_list<const kvp> list)
{
    std::ostringstream ss;
    ss << R"({"severity":"ERROR")";
    OutputJsonKvpToStream(ss, list);
    ss << "}";
    std::cerr << ss.str() << std::endl;
}

void info(std::initializer_list<const kvp> list)
{
    std::ostringstream ss;
    ss << R"({"severity":"INFO")";
    OutputJsonKvpToStream(ss, list);
    ss << "}";
    std::cerr << ss.str() << std::endl;
}

}

#endif