cmake_minimum_required(VERSION 3.16)
project(ganymede VERSION 0.1.0)

# Import third parties
include(${CMAKE_SOURCE_DIR}/cmake/conan/conan_paths.cmake)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(cpp-jwt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(mongoc-1.0 REQUIRED)
find_package(mongocxx REQUIRED)
find_package(cpr REQUIRED)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(GANYMEDE_GCP_PROJECT_ID ganymede-hydro)

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

    add_library(ganymede.services.${ADD_MICROSERVICE_NAME}.proto STATIC ${ADD_MICROSERVICE_PROTO})
    target_link_libraries(ganymede.services.${ADD_MICROSERVICE_NAME}.proto PUBLIC protobuf::libprotobuf gRPC::grpc gRPC::grpc++)
    target_compile_features(ganymede.services.${ADD_MICROSERVICE_NAME}.proto PUBLIC cxx_std_17)
    protobuf_generate(TARGET ganymede.services.${ADD_MICROSERVICE_NAME}.proto LANGUAGE cpp)
    protobuf_generate(TARGET ganymede.services.${ADD_MICROSERVICE_NAME}.proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

    add_executable(ganymede.services.${ADD_MICROSERVICE_NAME} ${ADD_MICROSERVICE_SOURCES} ${ADD_MICROSERVICE_HEADERS})

    target_compile_features(ganymede.services.${ADD_MICROSERVICE_NAME} PUBLIC cxx_std_17)
    target_compile_options(ganymede.services.${ADD_MICROSERVICE_NAME} PUBLIC -Wall -Wpedantic -Werror)
    target_include_directories(ganymede.services.${ADD_MICROSERVICE_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}) # To include generated Protobuf/gRPC files
    target_link_libraries(ganymede.services.${ADD_MICROSERVICE_NAME} PUBLIC ganymede.services.${ADD_MICROSERVICE_NAME}.proto ganymede.common)

    add_custom_target(
        ganymede.services.${ADD_MICROSERVICE_NAME}.docker
        COMMAND docker build -t gcr.io/${GANYMEDE_GCP_PROJECT_ID}/${ADD_MICROSERVICE_NAME}.service:${PROJECT_VERSION} -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile .
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endfunction()

add_subdirectory(ganymede)