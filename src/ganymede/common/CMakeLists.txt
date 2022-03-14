add_library(
    ganymede.common
    STATIC
    log.hh
    log.cc
    operation.hh
    status.cc
    status.hh
)

target_link_libraries(ganymede.common
    PUBLIC
        CONAN_PKG::nlohmann_json
        CONAN_PKG::grpc
)

target_standard_compile_flags(TARGET ganymede.common)

add_subdirectory(auth)
add_subdirectory(mongo)