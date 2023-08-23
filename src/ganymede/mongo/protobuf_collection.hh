#ifndef GANYMEDE__MONGO__PROTOBUF_COLLECTION_HH_
#define GANYMEDE__MONGO__PROTOBUF_COLLECTION_HH_

#include <memory>

#include <google/protobuf/message.h>

#include <mongocxx/database.hpp>

#include <ganymede/api/result.hh>

namespace ganymede::mongo {

class ProtobufCollectionUntyped {
private:
    template <class Message>
    friend class ProtobufCollection;

    ProtobufCollectionUntyped() = delete;
    ProtobufCollectionUntyped(mongocxx::collection&& collection);
    ~ProtobufCollectionUntyped();

    bool CreateUniqueIndex(int fieldNumber);

    api::Result<void> Contains(const std::string& oid, const std::string& domain);
    api::Result<void> Contains(const bsoncxx::document::view& filter);

    api::Result<void> GetDocument(const std::string& oid, const std::string& domain, google::protobuf::Message& output);
    api::Result<void> GetDocument(const bsoncxx::document::view& filter, google::protobuf::Message& output);

    api::Result<std::string> CreateDocument(const std::string& domain, const google::protobuf::Message& message);
    api::Result<void> UpdateDocument(const std::string& oid, const std::string& domain, const google::protobuf::Message& message);

    api::Result<void> DeleteDocument(const std::string& oid, const std::string& domain);

private:
    struct Priv;
    std::unique_ptr<Priv> d;
};

template <class Message>
class ProtobufCollection {
public:
    ProtobufCollection() = delete;
    ProtobufCollection(mongocxx::collection&& collection)
        : internal(std::move(collection))
    {
    }

    ~ProtobufCollection() = default;

    bool CreateUniqueIndex(int fieldNumber)
    {
        return internal.CreateUniqueIndex(fieldNumber);
    }

    api::Result<void> Contains(const std::string& oid, const std::string& domain)
    {
        return internal.Contains(oid, domain);
    }

    api::Result<void> Contains(const bsoncxx::document::view& filter)
    {
        return internal.Contains(filter);
    }

    api::Result<Message> GetDocument(const std::string& oid, const std::string& domain)
    {
        api::Result<Message> out{ Message() };
        internal.GetDocument(oid, domain, out.value());
        return out;
    }

    api::Result<Message> GetDocument(const bsoncxx::document::view& filter)
    {
        api::Result<Message> out{ Message() };
        internal.GetDocument(filter, out.value());
        return out;
    }

    api::Result<std::string> CreateDocument(const std::string& domain, const Message& message)
    {
        api::Result<Message> out{ Message() };
        auto result = internal.CreateDocument(domain, message);
        return result;
    }

    api::Result<Message> UpdateDocument(const std::string& oid, const std::string& domain, const Message& message)
    {
        api::Result<Message> out{ Message() };
        auto result = internal.UpdateDocument(oid, domain, message);

        if (result) {
            internal.GetDocument(oid, domain, out.value());
            return out;
        } else {
            return { result.status(), result.error() };
        }
    }

    api::Result<void> DeleteDocument(const std::string& oid, const std::string& domain)
    {
        return internal.DeleteDocument(oid, domain);
    }

private:
    ProtobufCollectionUntyped internal;
};

} // namespace ganymede::mongo

#endif