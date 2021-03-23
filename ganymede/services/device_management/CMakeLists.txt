project(ganymede.services.device_management VERSION 0.1.0)

add_microservice(
    NAME device_management
    HEADERS
    SOURCES
        src/main.cc
    PROTO
        device_management.proto
)