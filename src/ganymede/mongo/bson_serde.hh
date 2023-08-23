#ifndef GANYMEDE__MONGO__BSON_SERDE_HH_
#define GANYMEDE__MONGO__BSON_SERDE_HH_

#include <string>

#include <google/protobuf/message.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

namespace ganymede::mongo {

bool MessageToBson(const google::protobuf::Message& message, bsoncxx::builder::basic::document& builder);
bool BsonToMessage(const bsoncxx::document::view& doc, google::protobuf::Message& message);

} // namespace ganymede::mongo

#endif