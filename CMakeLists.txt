cmake_minimum_required(VERSION 3.17)
project(ganymede VERSION 0.1.0)

# Import third parties
list(APPEND CMAKE_PREFIX_PATH
    "third_party/grpc/cmake/install"
    "third_party/cpp-jwt/cmake/install"
    "third_party/json/cmake/install"
    "third_party/googletest/cmake/install"
    "third_party/mongo-c-driver/cmake/install"
    "third_party/mongo-cxx-driver/cmake/install"
)

find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(cpp-jwt REQUIRED)
find_package (mongoc-1.0 CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(GCLOUD_PROJECT_ID ganymede-hydro)
string(TIMESTAMP unix_time "%s")


# add_microservice
# Generic function to create a C++/gRPC/Docker microservice. Creates 2 targets,
# one for the executable itself and another for the Docker container. Generated
# targets are of the format
#   `ganymede.services.<name>`        (executable)
#   `ganymede.services.<name>.docker` (container)
# Also sets some default compilation flags : std=c++17, -Wall, -Werror and -Wpedantic
function(add_microservice)
    set(options )
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES HEADERS PROTO)
    cmake_parse_arguments(ADD_MICROSERVICE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    add_executable(ganymede.services.${ADD_MICROSERVICE_NAME} ${ADD_MICROSERVICE_SOURCES} ${ADD_MICROSERVICE_HEADERS} ${ADD_MICROSERVICE_PROTO})
    protobuf_generate(TARGET ganymede.services.${ADD_MICROSERVICE_NAME} LANGUAGE cpp)
    protobuf_generate(TARGET ganymede.services.${ADD_MICROSERVICE_NAME} LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

    target_compile_features(ganymede.services.${ADD_MICROSERVICE_NAME} PUBLIC cxx_std_17)
    target_compile_options(ganymede.services.${ADD_MICROSERVICE_NAME} PUBLIC -Wall -Werror -Wpedantic)
    target_include_directories(ganymede.services.${ADD_MICROSERVICE_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}) # To include generated Protobuf/gRPC files
    target_link_libraries(ganymede.services.${ADD_MICROSERVICE_NAME} PUBLIC protobuf::libprotobuf gRPC::grpc gRPC::grpc gRPC::grpc++)

    add_custom_target(
        ganymede.services.${ADD_MICROSERVICE_NAME}.docker
        COMMAND docker build -t gcr.io/${GCLOUD_PROJECT_ID}/${ADD_MICROSERVICE_NAME}.service:${CMAKE_PROJECT_VERSION} -f services/${ADD_MICROSERVICE_NAME}/Dockerfile cmake/build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endfunction()

add_subdirectory(services)