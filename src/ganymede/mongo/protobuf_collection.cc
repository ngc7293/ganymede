#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/document/view.hpp>

#include <mongocxx/exception/exception.hpp>

#include <ganymede/api/status.hh>
#include <ganymede/log/log.hh>
#include <ganymede/mongo/bson_serde.hh>
#include <ganymede/mongo/oid.hh>
#include <ganymede/mongo/protobuf_collection.hh>

namespace ganymede::mongo {

namespace {

const int MONGO_UNIQUE_KEY_VIOLATION = 11000;

bsoncxx::document::value BuildIDAndDomainFilter(const std::string& id, const std::string& domain)
{
    bsoncxx::builder::basic::document builder{};
    builder.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(id)));
    builder.append(bsoncxx::builder::basic::kvp("domain", domain));
    return builder.extract();
}

} // namespace

struct ProtobufCollectionUntyped::Priv {
    mongocxx::collection collection;
};

ProtobufCollectionUntyped::ProtobufCollectionUntyped(mongocxx::collection&& collection)
    : d(new Priv{ std::move(collection) })
{
}

ProtobufCollectionUntyped::~ProtobufCollectionUntyped() = default;

api::Result<void> ProtobufCollectionUntyped::Contains(const std::string& oid, const std::string& domain)
{
    if (not ValidateIsOid(oid)) {
        return { api::Status::INVALID_ARGUMENT, "invalid uid" };
    }

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

api::Result<void> ProtobufCollectionUntyped::Contains(const bsoncxx::document::view& filter)
{
    return d->collection.count_documents(filter) ? api::Status::OK : api::Status::NOT_FOUND;
}

api::Result<void> ProtobufCollectionUntyped::GetDocument(const std::string& oid, const std::string& domain, google::protobuf::Message& output)
{
    if (not ValidateIsOid(oid)) {
        return { api::Status::INVALID_ARGUMENT, "invalid uid" };
    }

    return GetDocument(BuildIDAndDomainFilter(oid, domain), output);
}

api::Result<void> ProtobufCollectionUntyped::GetDocument(const bsoncxx::document::view& filter, google::protobuf::Message& output)
{
    bsoncxx::stdx::optional<bsoncxx::document::value> result;

    result = d->collection.find_one(filter);

    if (result) {
        BsonToMessage(result.value(), output);
        return api::Status::OK;
    }

    return { api::Status::NOT_FOUND, "no such resource" };
}

api::Result<std::string> ProtobufCollectionUntyped::CreateDocument(const std::string& domain, const google::protobuf::Message& message)
{
    auto builder = bsoncxx::builder::basic::document{};
    builder.append(bsoncxx::builder::basic::kvp("domain", domain));

    if (not MessageToBson(message, builder)) {
        return api::Status::UNIMPLEMENTED;
    }

    bsoncxx::stdx::optional<mongocxx::result::insert_one> result;

    try {
        result = d->collection.insert_one(builder.view());
    } catch (const mongocxx::exception& e) {
        int ec = e.code().value();

        if (ec == MONGO_UNIQUE_KEY_VIOLATION) {
            return { api::Status::INVALID_ARGUMENT, "unique key collision" };
        } else {
            log::error(e.what(), { { "domain", domain } });
            return api::Status::INTERNAL;
        }
    }

    if (not result or result.value().result().inserted_count() != 1) {
        return api::Status::INTERNAL;
    }

    return { result->inserted_id().get_oid().value.to_string() };
}

api::Result<void> ProtobufCollectionUntyped::UpdateDocument(const std::string& oid, const std::string& domain, const google::protobuf::Message& message)
{
    if (not ValidateIsOid(oid)) {
        return api::Status::INVALID_ARGUMENT;
    }

    auto builder = bsoncxx::builder::basic::document{};
    auto nested = bsoncxx::builder::basic::document{};
    if (not MessageToBson(message, nested)) {
        return api::Status::UNIMPLEMENTED;
    }
    builder.append(bsoncxx::builder::basic::kvp("$set", nested));

    bsoncxx::stdx::optional<mongocxx::result::update> result;
    try {
        result = d->collection.update_one(BuildIDAndDomainFilter(oid, domain), builder.view());
    } catch (const mongocxx::exception& e) {
        log::error(e.what(), { { "domain", domain } });
        return api::Status::INTERNAL;
    }

    if (not result or result.value().matched_count() == 0) {
        return { api::Status::NOT_FOUND, "no such resource" };
    }

    return api::Status::OK;
}

api::Result<void> ProtobufCollectionUntyped::DeleteDocument(const std::string& oid, const std::string& domain)
{
    if (not ValidateIsOid(oid)) {
        return api::Status::INVALID_ARGUMENT;
    }

    bsoncxx::stdx::optional<mongocxx::result::delete_result> result;

    try {
        result = d->collection.delete_one(BuildIDAndDomainFilter(oid, domain));
    } catch (const mongocxx::exception& e) {
        log::error(e.what(), { { "domain", domain } });
        return api::Status::INTERNAL;
    }

    if (not result.has_value() or result.value().deleted_count() != 1) {
        return { api::Status::NOT_FOUND, "no such resource" };
    }

    return api::Status::OK;
}

} // namespace ganymede::mongo