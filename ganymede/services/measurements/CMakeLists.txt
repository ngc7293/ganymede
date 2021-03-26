project(ganymede.services.measurements VERSION 0.1.0)

add_microservice(
    NAME measurements
    HEADERS
    SOURCES
        src/main.cc
    PROTO
        measurements.proto
)