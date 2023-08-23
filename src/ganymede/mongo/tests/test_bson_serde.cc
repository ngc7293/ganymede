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
        R"({ "1" : "alpha", "2" : true, "3" : { "$numberLong" : "42" }, "4" : { "$numberDouble" : "3.1400000000000001243" } })"
    );
}

TEST(BsonSerdeTest, should_deserialize_simple_protobuf)
{
    auto input = bsoncxx::from_json(
        R"({ "1" : "bravo", "2" : false, "3" : { "$numberLong" : "24" }, "4" : { "$numberDouble" : "-0.0" } })"
    );

    TestMessageA message;
    ganymede::mongo::BsonToMessage(input, message);
    EXPECT_EQ(message.s1(), "bravo");
    EXPECT_EQ(message.b2(), false);
    EXPECT_EQ(message.i3(), 24);
    EXPECT_EQ(message.d4(), -0.0);
}