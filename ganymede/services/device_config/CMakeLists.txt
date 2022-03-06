project(ganymede.services.device_config VERSION 0.2.9)

add_microservice(
    NAME
        device_config
    HEADERS
        bson_serde.hh
    SOURCES
        main.cc
    PROTO
        device_config.proto
        device_config.service_config.proto
)

target_link_libraries(ganymede.services.device_config PUBLIC CONAN_PKG::mongo-cxx-driver CONAN_PKG::mongo-c-driver)