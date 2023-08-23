add_library(ganymede.auth STATIC
    auth_validator.cc
    auth_validator.hh
)

target_link_libraries(ganymede.auth
    PUBLIC
        CONAN_PKG::grpc
    PRIVATE
        CONAN_PKG::cpp-jwt
)

target_standard_compile_flags(TARGET ganymede.auth)