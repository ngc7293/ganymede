#include <gtest/gtest.h>

#include <bsoncxx/json.hpp>

#include <ganymede/mongo/bson_serde.hh>

#include "test.pb.h"

TEST(BsonSerdeTest, should_serialize_simple_protobuf)
{
    TestMessageA message;
    message.set_s1("alpha");
    message.set_b2(true);
    message.set_i3(42);
    message.set_d4(3.14);

    bsoncxx::builder::basic::document output;
    ganymede::mongo::MessageToBson(message, output);

    EXPECT_EQ(
        bsoncxx::to_json(output.view(), bsoncxx::ExtendedJsonMode::k_canonical),
        R"({ "1" : "alpha", "2" : true, "3" : { "$numberLong" : "42" }, "4" : { "$numberDouble" : "3.1400000000000001243" } })");
}

TEST(BsonSerdeTest, should_deserialize_simple_protobuf)
{
    auto input = bsoncxx::from_json(
        R"({ "1" : "bravo", "2" : false, "3" : { "$numberLong" : "24" }, "4" : { "$numberDouble" : "-0.0" } })");

    TestMessageA message;
    ganymede::mongo::BsonToMessage(input, message);
    EXPECT_EQ(message.s1(), "bravo");
    EXPECT_EQ(message.b2(), false);
    EXPECT_EQ(message.i3(), 24);
    EXPECT_EQ(message.d4(), -0.0);
}

TEST(BsonSerdeTest, should_serialize_nested_message)
{
    TestMessageB message;
    message.mutable_n3()->set_a(528491);
    message.mutable_n3()->set_b(0.1);

    bsoncxx::builder::basic::document output;
    ganymede::mongo::MessageToBson(message, output);

    EXPECT_EQ(
        bsoncxx::to_json(output.view(), bsoncxx::ExtendedJsonMode::k_canonical),
        R"({ "32" : { "1" : { "$numberLong" : "528491" }, "10" : { "$numberDouble" : "0.10000000000000000555" } } })");
}

TEST(BsonSerdeTest, should_deserialize_nested_message)
{
    auto input = bsoncxx::from_json(R"({ "32" : { "1" : { "$numberLong" : "31"} } })");

    TestMessageB message;
    ganymede::mongo::BsonToMessage(input, message);
    EXPECT_TRUE(message.has_n3());
    EXPECT_EQ(message.n3().a(), 31);
    EXPECT_EQ(message.n3().b(), 0.0);

    EXPECT_EQ(message.s1_size(), 0);
    EXPECT_EQ(message.n2_size(), 0);
}

TEST(BsonSerdeTest, should_serialize_repeated_field)
{
    TestMessageB message;
    message.mutable_s1()->Add("alpha");
    message.mutable_s1()->Add("bravo");
    message.mutable_s1()->Add("charlie");
    message.mutable_s1()->Add("delta");

    bsoncxx::builder::basic::document output;
    ganymede::mongo::MessageToBson(message, output);

    EXPECT_EQ(
        bsoncxx::to_json(output.view(), bsoncxx::ExtendedJsonMode::k_canonical),
        R"({ "30" : [ "alpha", "bravo", "charlie", "delta" ] })");
}

TEST(BsonSerdeTest, should_deserialize_repeated_field)
{
    auto input = bsoncxx::from_json(R"({ "30" : [ "alpha", "bravo", "charlie", "delta" ] })");

    TestMessageB message;
    ganymede::mongo::BsonToMessage(input, message);

    ASSERT_EQ(message.s1_size(), 4);
    EXPECT_EQ(message.s1(0), "alpha");
    EXPECT_EQ(message.s1(1), "bravo");
    EXPECT_EQ(message.s1(2), "charlie");
    EXPECT_EQ(message.s1(3), "delta");
}

TEST(BsonSerdeTest, should_serialize_repeated_message_field)
{
    TestMessageB message;
    {
        auto submessage = message.add_n2();
        submessage->set_a(1);
        submessage->set_b(2.0);
    }
    {
        auto submessage = message.add_n2();
        submessage->set_a(3);
        submessage->set_b(4.0);
    }

    bsoncxx::builder::basic::document output;
    ganymede::mongo::MessageToBson(message, output);

    EXPECT_EQ(
        bsoncxx::to_json(output.view(), bsoncxx::ExtendedJsonMode::k_canonical),
        R"({ "31" : [ { "1" : { "$numberLong" : "1" }, "10" : { "$numberDouble" : "2.0" } }, { "1" : { "$numberLong" : "3" }, "10" : { "$numberDouble" : "4.0" } } ] })");
}

TEST(BsonSerdeTest, should_deserialize_repeated_message_field)
{
    auto input = bsoncxx::from_json(R"({ "31" : [ { "1" : { "$numberLong" : "1" }, "10" : { "$numberDouble" : "2.0" } }, { "1" : { "$numberLong" : "3" }, "10" : { "$numberDouble" : "4.0" } } ] })");

    TestMessageB message;
    ganymede::mongo::BsonToMessage(input, message);

    ASSERT_EQ(message.n2_size(), 2);
    EXPECT_EQ(message.n2(0).a(), 1);
    EXPECT_EQ(message.n2(0).b(), 2.0);
    EXPECT_EQ(message.n2(1).a(), 3);
    EXPECT_EQ(message.n2(1).b(), 4.0);
}