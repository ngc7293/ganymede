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

if ("service.measurements" IN_LIST GANYMEDE_SCOPE)
    include(FetchContent)
    FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/whoshuu/cpr GIT_TAG aac5058a15e9ad5ad393973dc6fe44d7614a7f55) # the commit hash for 1.6.0
    set(BUILD_CPR_TESTS OFF)
    set(USE_SYSTEM_GTEST ON)
    FetchContent_MakeAvailable(cpr)
    
    target_link_libraries(ganymede.services.measurements PUBLIC cpr::cpr)
endif()