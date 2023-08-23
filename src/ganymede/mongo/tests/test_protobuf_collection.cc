#include <gtest/gtest.h>

#include <mongocxx/client.hpp>

#include <ganymede/mongo/protobuf_collection.hh>

#include "test.pb.h"

namespace {

const char* NULL_OID = "000000000000000000000000";

std::string nowstring()
{
    return std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

} // namespace

class ProtobufCollectionTest : public ::testing::Test {
public:
    ProtobufCollectionTest()
        : client_(mongocxx::uri{ "mongodb://localhost:27017/" })
        , database_(client_["test"])
        , collection_(database_["test"])
    {
    }

protected:
    mongocxx::client client_;
    mongocxx::database database_;
    ganymede::mongo::ProtobufCollection<TestMessageA> collection_;
};

TEST_F(ProtobufCollectionTest, should_check_if_collection_contains_document_by_id)
{
    auto result = collection_.Contains(NULL_OID, "domain");
    EXPECT_FALSE(result);
    EXPECT_EQ(result.status(), ganymede::api::Status::NOT_FOUND);

    auto id = collection_.CreateDocument("domain", TestMessageA());
    ASSERT_TRUE(id);
    EXPECT_TRUE(collection_.Contains(id.value(), "domain"));
}

TEST_F(ProtobufCollectionTest, should_check_if_collection_contains_document_by_filter)
{
    TestMessageA message;
    message.set_s1(nowstring());

    bsoncxx::builder::basic::document filter{};
    filter.append(bsoncxx::builder::basic::kvp("1", message.s1()));

    EXPECT_FALSE(collection_.Contains(filter));
    collection_.CreateDocument("domain", message);
    EXPECT_TRUE(collection_.Contains(filter));
}

TEST_F(ProtobufCollectionTest, should_remove_document)
{
    auto id = collection_.CreateDocument("domain", TestMessageA());

    ASSERT_TRUE(id);
    EXPECT_TRUE(collection_.Contains(id.value(), "domain"));
    EXPECT_TRUE(collection_.DeleteDocument(id.value(), "domain"));
    EXPECT_FALSE(collection_.Contains(id.value(), "domain"));
}

TEST_F(ProtobufCollectionTest, should_get_document_by_id)
{
    TestMessageA message;
    message.set_s1(nowstring());
    message.set_i3(72);

    auto id = collection_.CreateDocument("domain", message);
    ASSERT_TRUE(id);

    auto result = collection_.GetDocument(id.value(), "domain");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().s1(), message.s1());
    EXPECT_EQ(result.value().i3(), message.i3());
}

TEST_F(ProtobufCollectionTest, should_return_document_by_filter)
{
    TestMessageA message;
    message.set_s1(nowstring());
    message.set_i3(72);

    auto id = collection_.CreateDocument("domain", message);
    ASSERT_TRUE(id);

    bsoncxx::builder::basic::document filter{};
    filter.append(bsoncxx::builder::basic::kvp("1", message.s1()));

    auto result = collection_.GetDocument(filter);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().s1(), message.s1());
    EXPECT_EQ(result.value().i3(), message.i3());
}

TEST_F(ProtobufCollectionTest, should_return_not_found_if_document_does_not_exist)
{
    {
        auto result = collection_.GetDocument(NULL_OID, "domain");
        EXPECT_FALSE(result);
        EXPECT_EQ(result.status(), ganymede::api::Status::NOT_FOUND);
    }
    {
        auto result = collection_.UpdateDocument(NULL_OID, "domain", TestMessageA());
        EXPECT_FALSE(result);
        EXPECT_EQ(result.status(), ganymede::api::Status::NOT_FOUND);
    }
    {
        auto result = collection_.DeleteDocument(NULL_OID, "domain");
        EXPECT_FALSE(result);
        EXPECT_EQ(result.status(), ganymede::api::Status::NOT_FOUND);
    }
}

TEST_F(ProtobufCollectionTest, should_return_error_if_called_with_invalid_oid)
{
    {
        auto result = collection_.Contains("\0", "domain");
        EXPECT_FALSE(result);
        EXPECT_EQ(result.status(), ganymede::api::Status::INVALID_ARGUMENT);
    }
    {
        auto result = collection_.GetDocument("invalid", "domain");
        EXPECT_FALSE(result);
        EXPECT_EQ(result.status(), ganymede::api::Status::INVALID_ARGUMENT);
    }
    {
        auto result = collection_.UpdateDocument("00000000000g", "domain", TestMessageA());
        EXPECT_FALSE(result);
        EXPECT_EQ(result.status(), ganymede::api::Status::INVALID_ARGUMENT);
    }
    {
        auto result = collection_.DeleteDocument("weewoo", "domain");
        EXPECT_FALSE(result);
        EXPECT_EQ(result.status(), ganymede::api::Status::INVALID_ARGUMENT);
    }
}

TEST_F(ProtobufCollectionTest, should_update_document)
{
    TestMessageA message;
    message.set_s1(nowstring());
    message.set_i3(72);

    auto id = collection_.CreateDocument("domain", message);
    ASSERT_TRUE(id);

    message.set_s1(nowstring());
    message.set_i3(528491);

    {
        auto result = collection_.UpdateDocument(id.value(), "domain", message);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value().s1(), message.s1());
        EXPECT_EQ(result.value().i3(), message.i3());
    }
    {
        auto result = collection_.GetDocument(id.value(), "domain");
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value().s1(), message.s1());
        EXPECT_EQ(result.value().i3(), message.i3());
    }
}
