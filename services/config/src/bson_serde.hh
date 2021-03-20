#ifndef BSON_SERDE_HH_
#define BSON_SERDE_HH_

#include <string>

#include <google/protobuf/message.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

namespace ganymede {

bool MessageToBson(const google::protobuf::Message& message, bsoncxx::builder::basic::document& builder)
{
    const auto& descriptor = *(message.GetDescriptor());
    const auto& reflection = *(message.GetReflection());

    for (int i = 0; i < descriptor.field_count(); i++) {
        const auto& field = *(descriptor.field(i));

        if (reflection.HasField(message, &field)) {
            switch (field.cpp_type()) {
                default:
                    return false;

                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    builder.append(bsoncxx::builder::basic::kvp(std::to_string(field.number()), reflection.GetFloat(message, &field)));
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    builder.append(bsoncxx::builder::basic::kvp(std::to_string(field.number()), (std::int64_t) reflection.GetUInt32(message, &field)));
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    builder.append(bsoncxx::builder::basic::kvp(std::to_string(field.number()), reflection.GetString(message, &field)));
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    auto nested = bsoncxx::builder::basic::document();
                    MessageToBson(reflection.GetMessage(message, &field), nested);
                    builder.append(bsoncxx::builder::basic::kvp(std::to_string(field.number()), nested.extract()));
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
        const auto& field = *(descriptor.field(i));

        if (doc.find(std::to_string(field.number())) != doc.end()) {
            switch (field.cpp_type()) {
                default:
                    return false;

                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    reflection.SetFloat(&message, &field, (float) doc[std::to_string(field.number())].get_double().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    reflection.SetUInt32(&message, &field, (std::uint32_t) doc[std::to_string(field.number())].get_int64().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    reflection.SetString(&message, &field, std::string(doc[std::to_string(field.number())].get_utf8().value));
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    auto nested = reflection.MutableMessage(&message, &field);
                    BsonToMessage(doc[std::to_string(field.number())].get_document().value, *nested);
                    break;
            }
        }
    }

    return false;
}

}

#endif