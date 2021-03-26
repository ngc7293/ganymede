add_library(
    ganymede.common
    STATIC
    status.hh
    auth/jwt.hh
)

target_link_libraries(
    ganymede.common
    PUBLIC
    gRPC::grpc
    gRPC::grpc++
    cpp-jwt::cpp-jwt
)