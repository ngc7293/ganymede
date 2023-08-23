add_executable(ganymede.mongo.test
    main.cc
    test_bson_serde.cc
    test_protobuf_collection.cc
    test.proto
)

protobuf_generate(TARGET ganymede.mongo.test LANGUAGE cpp)
target_include_directories(ganymede.mongo.test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(ganymede.mongo.test
    PUBLIC
        ganymede.mongo
        ganymede.testing
        CONAN_PKG::gtest
)

target_standard_compile_flags(TARGET ganymede.mongo.test)
gtest_discover_tests(ganymede.mongo.test)