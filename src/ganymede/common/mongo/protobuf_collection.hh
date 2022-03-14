#ifndef GANYMEDE__COMMON__MONGO__PROTOBUF_COLLECTION_HH_
#define GANYMEDE__COMMON__MONGO__PROTOBUF_COLLECTION_HH_

#include <memory>

#include <mongocxx/database.hpp>

#include <google/protobuf/message.h>

#include <ganymede/common/operation.hh>

namespace ganymede::common::mongo {

class ProtobufCollectionUntyped {
private:
    template<class Message>
    friend class ProtobufCollection;

    ProtobufCollectionUntyped() = delete;
    ProtobufCollectionUntyped(mongocxx::collection&& collection);
    ~ProtobufCollectionUntyped();

    bool CreateUniqueIndex(int fieldNumber);

    Result<Void> Contains(const std::string& oid, const std::string& domain);
    Result<Void> Contains(const bsoncxx::document::view& filter);

    Result<Void> GetDocument(const std::string& oid, const std::string& domain, google::protobuf::Message& output);
    Result<Void> GetDocument(const bsoncxx::document::view& filter, google::protobuf::Message& output);

    Result<std::string> CreateDocument(const std::string& domain, const google::protobuf::Message& message);
    Result<Void> UpdateDocument(const std::string& oid, const std::string& domain, const google::protobuf::Message& message);

    Result<Void> DeleteDocument(const std::string& oid, const std::string& domain);
private:
    struct Priv;
    std::unique_ptr<Priv> d;
};

template<class Message>
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

    Result<Void> Contains(const std::string& oid, const std::string& domain)
    {
        return internal.Contains(oid, domain);
    }

    Result<Void> Contains(const bsoncxx::document::view& filter)
    {
        return internal.Contains(filter);
    }

    Result<Message> GetDocument(const std::string& oid, const std::string& domain)
    {
        Result<Message> out{Message()};
        internal.GetDocument(oid, domain, out.mutable_value());
        return out;
    }

    Result<Message> GetDocument(const bsoncxx::document::view& filter)
    {
        Result<Message> out{Message()};
        internal.GetDocument(filter, out.mutable_value());
        return out;
    }

    Result<Message> CreateDocument(const std::string& domain, const Message& message)
    {
        Result<Message> out{Message()};
        auto result = internal.CreateDocument(domain, message);

        if (result) {
            internal.GetDocument(result.value(), domain, out.mutable_value());
            return out;
        } else {
            return result.status();
        }
    }

    Result<Message> UpdateDocument(const std::string& oid, const std::string& domain, const Message& message)
    {
        Result<Message> out{Message()};
        auto result = internal.UpdateDocument(oid, domain, message);

        if (result) {
            internal.GetDocument(oid, domain, out.mutable_value());
            return out;
        } else {
            return result.status();
        }
    }

    Result<Void> DeleteDocument(const std::string& oid, const std::string& domain)
    {
        return internal.DeleteDocument(oid, domain);
    }

private:
    ProtobufCollectionUntyped internal;
};

}

#endif