if("service.config" IN_LIST GANYMEDE_SCOPE)
    add_subdirectory(config)
endif()

if("service.measurements" IN_LIST GANYMEDE_SCOPE)
    add_subdirectory(measurements)
endif()

if("service.device_management" IN_LIST GANYMEDE_SCOPE)
    add_subdirectory(device_management)
endif()