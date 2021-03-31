project(ganymede.services.measurements VERSION 0.1.1)

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
    FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/whoshuu/cpr.git GIT_TAG c8d33915dbd88ad6c92b258869b03aba06587ff9) # the commit hash for 1.5.0
    set(BUILD_CPR_TESTS OFF)
    set(USE_SYSTEM_GTEST ON)
    FetchContent_MakeAvailable(cpr)
    
    target_link_libraries(ganymede.services.measurements PUBLIC cpr::cpr)
endif()