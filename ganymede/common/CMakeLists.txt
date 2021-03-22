add_library(
    ganymede.common
    STATIC
    status.hh
)

target_link_libraries(ganymede.common gRPC::grpc gRPC::grpc++)