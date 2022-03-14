cmake_minimum_required(VERSION 3.16)
project(ganymede VERSION 0.1.0)

# Import third parties
include("${CMAKE_BINARY_DIR}/conan_paths.cmake")
include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_basic_setup(TARGETS)

set(grpc_cpp_plugin_location "${CONAN_GRPC_ROOT}/bin/grpc_cpp_plugin")
message("${grpc_cpp_plugin_location}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(GANYMEDE_GCP_PROJECT_ID ganymede-hydro)

function(target_standard_compile_flags)
    set(options )
    set(oneValueArgs TARGET)
    set(multiValueArgs FEATURES OPTIONS)
    cmake_parse_arguments(TARGET_STANDARD_COMPILE_FLAGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    target_compile_features(${TARGET_STANDARD_COMPILE_FLAGS_TARGET} PUBLIC cxx_std_20 ${TARGET_STANDARD_COMPILE_FLAGS_FEATURES})
    target_compile_options(${TARGET_STANDARD_COMPILE_FLAGS_TARGET} PUBLIC -Wall -Wextra -Wpedantic -Werror ${TARGET_STANDARD_COMPILE_FLAGS_OPTIONS})
endfunction()

# add_microservice
# Generic function to create a C++/gRPC/Docker microservice. Creates 2 targets,
# one for the executable itself and another for the Docker container. Generated
# targets are of the format
#   `ganymede.services.<name>.proto`  (compiled protos)
#   `ganymede.services.<name>.core`   (service implementation)
#   `ganymede.services.<name>`        (executable)
#   `ganymede.services.<name>.docker` (container)
# Also sets some default compilation flags : std=c++17, -Wall, -Werror and -Wpedantic
function(add_microservice)
    set(options )
    set(oneValueArgs NAME MAIN)
    set(multiValueArgs SOURCES HEADERS PROTO)
    cmake_parse_arguments(ADD_MICROSERVICE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    add_library(ganymede.services.${ADD_MICROSERVICE_NAME}.proto STATIC ${ADD_MICROSERVICE_PROTO})
    target_link_libraries(ganymede.services.${ADD_MICROSERVICE_NAME}.proto PUBLIC CONAN_PKG::grpc)
    target_compile_features(ganymede.services.${ADD_MICROSERVICE_NAME}.proto PUBLIC cxx_std_20)
    protobuf_generate(TARGET ganymede.services.${ADD_MICROSERVICE_NAME}.proto LANGUAGE cpp)
    protobuf_generate(TARGET ganymede.services.${ADD_MICROSERVICE_NAME}.proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

    add_library(ganymede.services.${ADD_MICROSERVICE_NAME}.core STATIC ${ADD_MICROSERVICE_SOURCES} ${ADD_MICROSERVICE_HEADERS})
    target_standard_compile_flags(TARGET ganymede.services.${ADD_MICROSERVICE_NAME}.core)
    target_include_directories(ganymede.services.${ADD_MICROSERVICE_NAME}.core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}) # To include generated Protobuf/gRPC files
    target_link_libraries(ganymede.services.${ADD_MICROSERVICE_NAME}.core PUBLIC ganymede.services.${ADD_MICROSERVICE_NAME}.proto ganymede.common ganymede.common.auth)
    
    add_executable(ganymede.services.${ADD_MICROSERVICE_NAME} ${ADD_MICROSERVICE_MAIN})
    # target_include_directories(ganymede.services.${ADD_MICROSERVICE_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}) # To include generated Protobuf/gRPC files
    target_link_libraries(ganymede.services.${ADD_MICROSERVICE_NAME} PUBLIC ganymede.services.${ADD_MICROSERVICE_NAME}.core)

    add_custom_target(
        ganymede.services.${ADD_MICROSERVICE_NAME}.docker
        COMMAND docker build -t gcr.io/${GANYMEDE_GCP_PROJECT_ID}/${ADD_MICROSERVICE_NAME}.service:${PROJECT_VERSION} -f ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile .
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endfunction()

add_subdirectory(libs)
add_subdirectory(src)