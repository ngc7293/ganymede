#ifndef GANYMEDE__MONGO__OID_HH_
#define GANYMEDE__MONGO__OID_HH_

#include <string>

namespace ganymede::mongo {

bool ValidateIsOid(const std::string& id);

} // namespace ganymede::mongo

#endif