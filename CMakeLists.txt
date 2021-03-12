cmake_minimum_required(VERSION 3.17)
project(ganymede VERSION 0.1.0)

# Import third parties
list(APPEND CMAKE_PREFIX_PATH
    "third_party/grpc/cmake/install"
    "third_party/cpp-jwt/cmake/install"
    "third_party/json/cmake/install"
    "third_party/googletest/cmake/install"
)

find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpp-jwt REQUIRED)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

# Set output structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(services)