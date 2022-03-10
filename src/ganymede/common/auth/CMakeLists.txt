add_library(ganymede.common.auth STATIC
    jwt.cc
    jwt.hh
)

target_link_libraries(ganymede.common.auth
    PUBLIC
        CONAN_PKG::grpc
    PRIVATE
        CONAN_PKG::cpp-jwt
)

target_standard_compile_flags(TARGET ganymede.common.auth)