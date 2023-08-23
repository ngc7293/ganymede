#ifndef GANYMEDE__MONGO__OID_HH_
#define GANYMEDE__MONGO__OID_HH_

#include <string>

#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/types/bson_value/view.hpp>

namespace ganymede::mongo {

bool ValidateIsOid(const std::string& id);

std::string OIDToString(const bsoncxx::types::bson_value::view& val);
std::string GetDocumentID(const bsoncxx::document::view_or_value& view);

}

#endif