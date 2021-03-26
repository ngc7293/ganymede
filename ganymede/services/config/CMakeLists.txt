project(ganymede.services.config VERSION 0.2.0)

list(APPEND CMAKE_PREFIX_PATH
    "${CMAKE_SOURCE_DIR}/third_party/mongo-c-driver/cmake/install"
    "${CMAKE_SOURCE_DIR}/third_party/mongo-cxx-driver/cmake/install"
)

find_package(mongoc-1.0 CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)

add_microservice(
    NAME config
    HEADERS
    SOURCES
        src/main.cc
    PROTO
        config.proto
        config.service_config.proto
)

target_link_libraries(ganymede.services.config PUBLIC mongo::mongocxx_shared mongo::mongoc_shared)
file(COPY config.service_config.json DESTINATION ${CMAKE_BINARY_DIR}/config/)