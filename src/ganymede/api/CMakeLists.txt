add_library(ganymede.api
    result.hh
    status.hh
    void.cc
)

target_link_libraries(ganymede.api
    PUBLIC
        CONAN_PKG::grpc
)

target_standard_compile_flags(TARGET ganymede.api)