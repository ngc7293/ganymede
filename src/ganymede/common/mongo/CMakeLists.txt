add_library(ganymede.common.mongo STATIC
    bson_serde.cc
    bson_serde.hh
    oid.hh
    oid.cc
)

target_link_libraries(ganymede.common.mongo
    PUBLIC
        CONAN_PKG::grpc
        CONAN_PKG::mongo-cxx-driver
        CONAN_PKG::mongo-c-driver
)

target_standard_compile_flags(TARGET ganymede.common.mongo)
