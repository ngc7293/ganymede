add_microservice(
    NAME config
    HEADERS
    SOURCES
        src/main.cc
    PROTO
        config.proto
        config.service_config.proto
)

target_link_libraries(ganymede.services.config PUBLIC mongo::mongocxx_shared)