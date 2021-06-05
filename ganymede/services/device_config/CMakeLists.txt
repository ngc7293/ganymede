project(ganymede.services.device_config VERSION 0.2.9)

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
    find_package(mongoc-1.0 REQUIRED)
    find_package(mongocxx REQUIRED)
    target_link_libraries(ganymede.services.device_config PUBLIC mongo::mongocxx_static mongo::mongoc_static)
endif()