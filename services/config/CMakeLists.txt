
set(HEADERS
)

set(SOURCES
    src/main.cc
)

set(PROTOS
    config_service.proto
)

add_executable(ganymede.services.config ${SOURCES} ${HEADERS} ${PROTOS})
protobuf_generate(TARGET ganymede.services.config LANGUAGE cpp)
protobuf_generate(TARGET ganymede.services.config LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

target_compile_features(ganymede.services.config PUBLIC cxx_std_17)
target_compile_options(ganymede.services.config PUBLIC -Wall -Werror -Wpedantic)
target_include_directories(ganymede.services.config PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(ganymede.services.config PUBLIC protobuf::libprotobuf gRPC::grpc gRPC::grpc gRPC::grpc++)