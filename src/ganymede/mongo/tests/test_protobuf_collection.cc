#include <gtest/gtest.h>

#include <mongocxx/client.hpp>

#include <ganymede/mongo/protobuf_collection.hh>

#include "test.pb.h"

namespace {

std::string
nowstring()
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
    EXPECT_FALSE(collection_.Contains("000000000000", "domain"));
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

TEST_F(ProtobufCollectionTest, should_return_document_by_id)
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