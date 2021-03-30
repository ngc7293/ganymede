add_library(
    ganymede.common
    STATIC
    status.hh
    log.hh
    log.cc
    auth/jwt.hh
)

target_link_libraries(
    ganymede.common
    PUBLIC
    gRPC::grpc
    gRPC::grpc++
    cpp-jwt::cpp-jwt
    nlohmann_json::nlohmann_json
)

target_compile_features(ganymede.common PUBLIC cxx_std_20)