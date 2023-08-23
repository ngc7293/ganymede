add_library(ganymede.testing STATIC
    docker.hh
    mock_auth_validator.hh
    service_test.hh
    void.cc
)

target_link_libraries(ganymede.testing
    PUBLIC
        CONAN_PKG::grpc
        CONAN_PKG::gtest
        CONAN_PKG::libcurl
        CONAN_PKG::nlohmann_json
)