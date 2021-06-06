project(ganymede.services.measurements VERSION 0.1.2)

add_microservice(
    NAME measurements
    HEADERS
        src/influxdb.hh
    SOURCES
        src/main.cc
        src/influxdb.cc
    PROTO
        measurements.proto
        measurements.service_config.proto
)

target_link_libraries(ganymede.services.measurements PUBLIC cpr::cpr)