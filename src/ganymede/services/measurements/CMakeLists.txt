project(ganymede.services.measurements VERSION 0.1.2)

add_microservice(
    NAME measurements
    HEADERS
    SOURCES
        void.cc
    MAIN
        main.cc
    PROTO
        measurements.proto
        measurements.service_config.proto
)

target_link_libraries(ganymede.services.measurements PRIVATE influx)