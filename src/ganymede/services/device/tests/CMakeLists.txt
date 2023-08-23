add_executable(ganymede.services.device.test
    main.cc
    test_device_service_impl.cc
)

target_link_libraries(ganymede.services.device.test
    PUBLIC
        CONAN_PKG::gtest
)

gtest_discover_tests(ganymede.services.device.test)