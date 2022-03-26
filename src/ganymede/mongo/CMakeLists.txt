add_library(ganymede.mongo STATIC
    bson_serde.cc
    bson_serde.hh
    oid.hh
    oid.cc
    protobuf_collection.cc
    protobuf_collection.hh
)

target_link_libraries(ganymede.mongo
    PUBLIC
        ganymede.api
        CONAN_PKG::mongo-cxx-driver
        CONAN_PKG::mongo-c-driver
    PRIVATE
        ganymede.log
        CONAN_PKG::grpc
)

target_standard_compile_flags(TARGET ganymede.mongo)
