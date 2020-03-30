cmake_minimum_required(VERSION 3.10)

# set the project name
project(MiBridge)

set(MIBRIDGE_SRC
    mibridge.cpp
    SmartLight.cpp
    MiLight.cpp
    HeiLight.cpp
    )

# add the executable
add_executable(mibridge ${MIBRIDGE_SRC})

# Add openmilight
add_subdirectory(external/openmilight)
target_include_directories(mibridge PUBLIC
                          "${PROJECT_SOURCE_DIR}/external/openmilight"
                          )

# Add MQTT-C library
set(MQTT_C_EXAMPLES OFF CACHE BOOL "disable examples")
add_subdirectory(external/MQTT-C)

# Add JSONCPP library
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile and (for jsoncpp_check) run JsonCpp test executables")
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Automatically run unit-tests as a post build step")
set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Generate and install .pc files")
add_subdirectory(external/jsoncpp)

# Add RF24 library (note rf24-bcm is an optimized version for Raspberry Pi)
find_library( RF24 NAMES librf24.a rf24-bcm rf24)
if (${RF24} STREQUAL "RF24-NOTFOUND")
    message(FATAL_ERROR "RF24 library not found!")
else()
    message(STATUS "RF24 library found!")
endif()

# add external libraries
set(EXTERNAL_LIBS
    OpenMilight
    jsoncpp_lib_static
    mqttc
    ${RF24}
    )

target_link_libraries(mibridge PUBLIC ${EXTERNAL_LIBS})
