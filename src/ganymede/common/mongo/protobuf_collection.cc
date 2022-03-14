#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <mongocxx/exception/exception.hpp>

#include <ganymede/common/log.hh>
#include <ganymede/common/mongo/bson_serde.hh>
#include <ganymede/common/mongo/protobuf_collection.hh>
#include <ganymede/common/mongo/oid.hh>
#include <ganymede/common/status.hh>

namespace ganymede::common::mongo {

namespace {
    const int MONGO_UNIQUE_KEY_VIOLATION = 11000;

    bsoncxx::builder::basic::document DocumentWithDomain(const std::string& domain)
    {
        auto builder = bsoncxx::builder::basic::document{};
        builder.append(bsoncxx::builder::basic::kvp("domain", domain));
        return builder;
    }

    bsoncxx::document::value BuildIDAndDomainFilter(const std::string& id, const std::string& domain)
    {
        auto builder = DocumentWithDomain(domain);
        builder.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(id.c_str(), id.length())));
        return builder.extract();
    }
}

struct ProtobufCollectionUntyped::Priv {
    mongocxx::collection collection;
};

ProtobufCollectionUntyped::ProtobufCollectionUntyped(mongocxx::collection&& collection)
    : d(new Priv{std::move(collection)})
{
}

ProtobufCollectionUntyped::~ProtobufCollectionUntyped() = default;

Result<Void> ProtobufCollectionUntyped::Contains(const std::string& oid, const std::string& domain)
{
    return Contains(BuildIDAndDomainFilter(oid, domain));
}

bool ProtobufCollectionUntyped::CreateUniqueIndex(int fieldNumber)
{
    auto keys = bsoncxx::builder::basic::document{};
    keys.append(bsoncxx::builder::basic::kvp(std::to_string(fieldNumber), 1));

    auto constraints = bsoncxx::builder::basic::document{};
    constraints.append(bsoncxx::builder::basic::kvp("unique", true));

    d->collection.create_index(keys.view(), constraints.view());
    return true;
}

Result<Void> ProtobufCollectionUntyped::Contains(const bsoncxx::document::view& filter)
{
    return d->collection.count_documents(filter) ? grpc::Status::OK : common::status::BAD_PAYLOAD;
}

Result<Void> ProtobufCollectionUntyped::GetDocument(const std::string& oid, const std::string& domain, google::protobuf::Message& output)
{
    if (not ValidateIsOid(oid)) {
        return {grpc::StatusCode::INVALID_ARGUMENT, "Invalid request"};
    }

    return GetDocument(BuildIDAndDomainFilter(oid, domain), output);
}

Result<Void> ProtobufCollectionUntyped::GetDocument(const bsoncxx::document::view& filter, google::protobuf::Message& output)
{
    bsoncxx::stdx::optional<bsoncxx::document::value> result;

    result = d->collection.find_one(filter);

    if (result) {
        BsonToMessage(result.value(), output);
        return {grpc::Status::OK};
    }

    return {common::status::NOT_FOUND};
}

Result<std::string> ProtobufCollectionUntyped::CreateDocument(const std::string& domain, const google::protobuf::Message& message)
{
    auto builder = bsoncxx::builder::basic::document{};
    builder.append(bsoncxx::builder::basic::kvp("domain", bsoncxx::oid(domain.c_str(), domain.length())));

    if (not MessageToBson(message, builder)) {
        return common::status::UNIMPLEMENTED;
    }

    bsoncxx::stdx::optional<mongocxx::result::insert_one> result;

    try {
        result = d->collection.insert_one(builder.view());
    } catch (const mongocxx::exception& e) {
        int ec = e.code().value();

        if (ec == MONGO_UNIQUE_KEY_VIOLATION) {
            return common::status::BAD_PAYLOAD;
        } else {
            common::log::error({{"message", e.what()}});
            return common::status::DATABASE_ERROR;
        }
    }

    if (not result or result.value().result().inserted_count() != 1) {
        return common::status::DATABASE_ERROR;
    }

    return {OIDToString(result->inserted_id())};
}

Result<Void> ProtobufCollectionUntyped::UpdateDocument(const std::string& oid, const std::string& domain, const google::protobuf::Message& message)
{
    if (not ValidateIsOid(oid)) {
        return common::status::BAD_PAYLOAD;
    }

    auto builder = bsoncxx::builder::basic::document{};
    auto nested = bsoncxx::builder::basic::document{};
    if (not MessageToBson(message, nested)) {
        return common::status::UNIMPLEMENTED;
    }
    builder.append(bsoncxx::builder::basic::kvp("$set", nested));

    bsoncxx::stdx::optional<mongocxx::result::update> result;
    try {
        result = d->collection.update_one(BuildIDAndDomainFilter(oid, domain), builder.view());
    } catch (const mongocxx::exception& e) {
        common::log::error({{"message", e.what()}});
        return common::status::DATABASE_ERROR;
    }

    if (not result or result.value().modified_count() == 0) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
    }

    return grpc::Status::OK;
}

Result<Void> ProtobufCollectionUntyped::DeleteDocument(const std::string& oid, const std::string& domain)
{
    if (not ValidateIsOid(oid)) {
        return common::status::BAD_PAYLOAD;
    }

    bsoncxx::stdx::optional<mongocxx::result::delete_result> result;

    try {
        result = d->collection.delete_one(BuildIDAndDomainFilter(oid, domain));
    } catch (const mongocxx::exception& e) {
        common::log::error({{"message", e.what()}});
        return common::status::DATABASE_ERROR;
    }

    if (not result.has_value() or result.value().deleted_count() != 1) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "no such device");
    }

    return grpc::Status::OK;
}

}