add_library(
    ganymede.common
    STATIC
    status.hh
    log.hh
    log.cc
    auth/jwt.hh
    auth/jwt.cc
)

target_link_libraries(ganymede.common PUBLIC CONAN_PKG::cpp-jwt CONAN_PKG::nlohmann_json CONAN_PKG::grpc)
target_compile_features(ganymede.common PUBLIC cxx_std_20)