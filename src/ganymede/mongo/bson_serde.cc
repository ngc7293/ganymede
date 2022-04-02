#ifndef GANYMEDE_COMMON_BSON_SERDE_HH_
#define GANYMEDE_COMMON_BSON_SERDE_HH_

#include <string>

#include <google/protobuf/message.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

namespace ganymede::mongo {

bool MessageToBson(const google::protobuf::Message& message, bsoncxx::builder::basic::document& builder)
{
    const auto& descriptor = *(message.GetDescriptor());
    const auto& reflection = *(message.GetReflection());

    for (int i = 0; i < descriptor.field_count(); i++) {
        const auto& field = *(descriptor.field(i));
        std::string field_tag = std::to_string(field.number());

        if (reflection.HasField(message, &field)) {
            switch (field.cpp_type()) {
            default:
                assert(false);
                return false;

            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                builder.append(bsoncxx::builder::basic::kvp(field_tag, reflection.GetFloat(message, &field)));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                builder.append(bsoncxx::builder::basic::kvp(field_tag, (std::int64_t)reflection.GetUInt32(message, &field)));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                builder.append(bsoncxx::builder::basic::kvp(field_tag, reflection.GetInt64(message, &field)));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                builder.append(bsoncxx::builder::basic::kvp(field_tag, reflection.GetString(message, &field)));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                auto nested = bsoncxx::builder::basic::document();
                MessageToBson(reflection.GetMessage(message, &field), nested);
                builder.append(bsoncxx::builder::basic::kvp(field_tag, nested.extract()));
                break;
            }
        }
    }

    return true;
}

bool BsonToMessage(const bsoncxx::document::view& doc, google::protobuf::Message& message)
{
    const auto& descriptor = *(message.GetDescriptor());
    const auto& reflection = *(message.GetReflection());

    message.Clear();

    for (int i = 0; i < descriptor.field_count(); i++) {
        const auto* field = descriptor.field(i);

        if (field == nullptr) {
            continue;
        }

        bsoncxx::document::view::const_iterator it = doc.find(std::to_string(field->number()));
        if (it != doc.end()) {
            switch (field->cpp_type()) {
            default:
                assert(false);
                return false;

            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                reflection.SetFloat(&message, field, (float)it->get_double().value);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                reflection.SetUInt32(&message, field, (std::uint32_t)it->get_int64().value);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                reflection.SetInt64(&message, field, it->get_int64().value);
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                reflection.SetString(&message, field, std::string(it->get_utf8().value));
                break;

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                auto nested = reflection.MutableMessage(&message, field);
                BsonToMessage(it->get_document().value, *nested);
                break;
            }
        }
    }

    return false;
}

} // namespace ganymede::mongo

#endif