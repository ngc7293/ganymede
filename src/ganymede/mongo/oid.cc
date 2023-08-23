#include <bson/bson.h>

#include <ganymede/mongo/oid.hh>

namespace ganymede::mongo {

bool ValidateIsOid(const std::string& id)
{
    return bson_oid_is_valid(id.c_str(), id.length());
}

} // namespace ganymede::mongo