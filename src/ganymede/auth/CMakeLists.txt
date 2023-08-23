add_library(ganymede.auth STATIC
    jwt.cc
    jwt.hh
)

target_link_libraries(ganymede.auth
    PUBLIC
        CONAN_PKG::grpc
    PRIVATE
        CONAN_PKG::cpp-jwt
)

target_standard_compile_flags(TARGET ganymede.auth)