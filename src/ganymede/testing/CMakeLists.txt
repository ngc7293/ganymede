add_library(ganymede.testing STATIC
    docker.hh
    void.cc
)

target_link_libraries(ganymede.testing
    PUBLIC
        CONAN_PKG::libcurl
        CONAN_PKG::nlohmann_json
)