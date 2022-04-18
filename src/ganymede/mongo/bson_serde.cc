/** BSON SerDe
 *
 * Defines 2 functions to convert BSON into Protobuf and vice-versa.
 *
 * This file boils down to a series of switch cases to handle each type
 * independently. It is ugly but I could'nt figure out a better way :(
 */

#ifndef GANYMEDE_COMMON_BSON_SERDE_HH_
#define GANYMEDE_COMMON_BSON_SERDE_HH_

#include <string>

#include <google/protobuf/message.h>
#include <google/protobuf/reflection.h>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "bson_serde.hh"

namespace ganymede::mongo {

namespace {
void AppendProtobufAsBsonKvp(bsoncxx::builder::basic::document& doc, const google::protobuf::Message& message, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field)
{
    std::string key{ std::to_string(field.number()) };

    switch (field.cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        doc.append(bsoncxx::builder::basic::kvp(key, reflection.GetFloat(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        doc.append(bsoncxx::builder::basic::kvp(key, reflection.GetDouble(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        doc.append(bsoncxx::builder::basic::kvp(key, reflection.GetBool(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        doc.append(bsoncxx::builder::basic::kvp(key, (std::int64_t)reflection.GetUInt32(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        doc.append(bsoncxx::builder::basic::kvp(key, (std::int64_t)reflection.GetUInt64(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        doc.append(bsoncxx::builder::basic::kvp(key, reflection.GetInt32(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64: // BSON does not support unsigned ints, so we use a i64. This reduces the maximum size of u64 to u63
        doc.append(bsoncxx::builder::basic::kvp(key, reflection.GetInt64(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        doc.append(bsoncxx::builder::basic::kvp(key, reflection.GetEnumValue(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        doc.append(bsoncxx::builder::basic::kvp(key, reflection.GetString(message, &field)));
        break;
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
        auto nested = bsoncxx::builder::basic::document();
        MessageToBson(reflection.GetMessage(message, &field), nested);
        doc.append(bsoncxx::builder::basic::kvp(key, nested.extract()));
    } break;
    default:
        assert(false);
    }
}

template <typename Type, typename CastType = Type>
bsoncxx::array::value ProtoRepeatedToBsonArray(const google::protobuf::RepeatedFieldRef<Type>& field)
{
    bsoncxx::builder::basic::array out;
    for (const Type& value : field) {
        out.append(static_cast<CastType>(value));
    }
    return out.extract();
}

template <>
bsoncxx::array::value ProtoRepeatedToBsonArray(const google::protobuf::RepeatedFieldRef<google::protobuf::Message>& field)
{
    bsoncxx::builder::basic::array out;
    for (const google::protobuf::Message& value : field) {
        auto nested = bsoncxx::builder::basic::document();
        MessageToBson(value, nested);
        out.append(nested.extract());
    }
    return out.extract();
}

bsoncxx::array::value ProtoRepeatedToBsonArray(const google::protobuf::Message& message, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field)
{
    switch (field.cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<float>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<double>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<bool>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        return ProtoRepeatedToBsonArray<uint32_t, int64_t>(reflection.GetRepeatedFieldRef<uint32_t>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        return ProtoRepeatedToBsonArray<uint64_t, int64_t>(reflection.GetRepeatedFieldRef<uint64_t>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<int32_t>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<int64_t>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<int64_t>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<std::string>(message, &field));
    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        return ProtoRepeatedToBsonArray(reflection.GetRepeatedFieldRef<google::protobuf::Message>(message, &field));

    default:
        assert(false);
        return bsoncxx::builder::basic::array().extract();
    }
}

void AppendProtobufAsBsonArray(bsoncxx::builder::basic::document& doc, const google::protobuf::Message& message, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field)
{
    std::string key{ std::to_string(field.number()) };
    doc.append(bsoncxx::builder::basic::kvp(key, ProtoRepeatedToBsonArray(message, reflection, field)));
}

void FillProtoFromBsonArray(google::protobuf::Message& message, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field, const bsoncxx::array::view& array)
{
    for (const auto& value : array) {
        switch (field.cpp_type()) {
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            reflection.AddFloat(&message, &field, (float)value.get_double().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            reflection.AddDouble(&message, &field, value.get_double().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            reflection.AddBool(&message, &field, value.get_bool().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            reflection.AddUInt32(&message, &field, (std::uint32_t)value.get_int64().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            reflection.AddUInt64(&message, &field, (std::uint64_t)value.get_int64().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            reflection.AddInt64(&message, &field, value.get_int32().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            reflection.AddInt64(&message, &field, value.get_int64().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
            reflection.AddEnumValue(&message, &field, value.get_int64().value);
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            reflection.AddString(&message, &field, std::string(value.get_utf8().value));
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
            auto nested = reflection.AddMessage(&message, &field);
            BsonToMessage(value.get_document().value, *nested);
        } break;
        default:
            assert(false);
        }
    }
}

}

bool MessageToBson(const google::protobuf::Message& message, bsoncxx::builder::basic::document& doc)
{
    const auto& descriptor = *(message.GetDescriptor());
    const auto& reflection = *(message.GetReflection());

    for (int i = 0; i < descriptor.field_count(); i++) {
        const auto& field = *(descriptor.field(i));

        if (field.is_repeated()) {
            if (reflection.FieldSize(message, &field) != 0) {
                AppendProtobufAsBsonArray(doc, message, reflection, field);
            }
        } else {
            if (reflection.HasField(message, &field)) {
                AppendProtobufAsBsonKvp(doc, message, reflection, field);
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
            if (field->is_repeated()) {
                FillProtoFromBsonArray(message, reflection, *field, it->get_array());
            } else {
                switch (field->cpp_type()) {
                default:
                    assert(false);
                    return false;

                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    reflection.SetFloat(&message, field, (float)it->get_double().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                    reflection.SetDouble(&message, field, it->get_double().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    reflection.SetBool(&message, field, it->get_bool().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    reflection.SetUInt32(&message, field, (std::uint32_t)it->get_int64().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    reflection.SetUInt64(&message, field, (std::uint64_t)it->get_int64().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                    reflection.SetInt64(&message, field, it->get_int32().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                    reflection.SetInt64(&message, field, it->get_int64().value);
                    break;

                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    reflection.SetEnumValue(&message, field, it->get_int64().value);
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
    }

    return false;
}

} // namespace ganymede::mongo

#endif