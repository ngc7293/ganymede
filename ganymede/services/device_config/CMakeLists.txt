project(ganymede.services.device_config VERSION 0.2.6)

add_microservice(
    NAME
        device_config
    HEADERS
    SOURCES
        src/main.cc
    PROTO
        device_config.proto
        device_config.service_config.proto
)

if("service.device_config" IN_LIST GANYMEDE_SCOPE)
    list(APPEND CMAKE_PREFIX_PATH
        "${CMAKE_SOURCE_DIR}/third_party/mongo-c-driver/cmake/install"
        "${CMAKE_SOURCE_DIR}/third_party/mongo-cxx-driver/cmake/install"
    )

    find_package(mongoc-1.0 CONFIG REQUIRED)
    find_package(mongocxx CONFIG REQUIRED)
    target_link_libraries(ganymede.services.device_config PUBLIC mongo::mongocxx_shared mongo::mongoc_shared)
endif()