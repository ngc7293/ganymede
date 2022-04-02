#include <bson/bson.h>

#include <ganymede/mongo/oid.hh>

namespace ganymede::mongo {

bool ValidateIsOid(const std::string& id)
{
    return bson_oid_is_valid(id.c_str(), id.length());
}

std::string OIDToString(const bsoncxx::types::bson_value::view& val)
{
    return val.get_oid().value.to_string();
}

std::string GetDocumentID(const bsoncxx::document::view_or_value& view)
{
    return OIDToString(view.view()["_id"].get_value());
}

} // namespace ganymede::mongo