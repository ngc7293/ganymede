project(ganymede.services.device VERSION 0.3.0)

add_microservice(
    NAME
        device
    HEADERS
        device_service_impl.hh
    SOURCES
        device_service_impl.cc
    MAIN
        main.cc
    PROTO
        device.proto
        device.config.proto
)

target_link_libraries(ganymede.services.device.core
    PUBLIC
        ganymede.mongo
        CONAN_PKG::date
        CONAN_PKG::mongo-cxx-driver
        CONAN_PKG::mongo-c-driver
)

add_subdirectory(tests)