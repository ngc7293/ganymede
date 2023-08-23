add_library(ganymede.log
    log.cc
    log.hh
)

target_link_libraries(ganymede.log
    PUBLIC
        CONAN_PKG::nlohmann_json
)

target_standard_compile_flags(TARGET ganymede.log)