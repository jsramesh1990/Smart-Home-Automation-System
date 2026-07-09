# SmartLock Pro - ESP32 Firmware CMake Configuration
# This file is used by ESP-IDF build system

# ============================================================
# PROJECT DEFINITION
# ============================================================

# Set minimum CMake version
cmake_minimum_required(VERSION 3.16)

# Define the project name and version
project(smartlock_pro)
set(PROJECT_VER "1.0.0")

# ============================================================
# COMPONENT REGISTRATION
# ============================================================

# Include the ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Additional component paths (if needed)
# set(EXTRA_COMPONENT_DIRS
#     ${CMAKE_CURRENT_SOURCE_DIR}/components
# )

# ============================================================
# BUILD CONFIGURATION
# ============================================================

# Set build type (Release/Debug)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Og -g3 -ggdb)
    add_compile_definitions(DEBUG_MODE)
else()
    add_compile_options(-Os -ffunction-sections -fdata-sections)
    add_compile_definitions(NDEBUG)
endif()

# Warning flags
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Wno-format-truncation
)

# ============================================================
# COMPONENT CONFIGURATION
# ============================================================

# Main component
idf_component_register(
    SRCS
        # Main application
        src/main.cpp
        
        # Core modules
        src/wifi_manager.cpp
        src/ble_manager.cpp
        src/lock_controller.cpp
        src/sensor_manager.cpp
        src/mqtt_client.cpp
        
        # Authentication modules
        src/authentication/fingerprint.cpp
        src/authentication/rfid.cpp
        src/authentication/keypad.cpp
        
        # Security modules
        src/security/encryption.cpp
        src/security/otp_generator.cpp
        
        # Utilities
        src/utilities/logger.cpp
        src/utilities/eeprom_manager.cpp
        
    INCLUDE_DIRS
        include
        src
        src/authentication
        src/security
        src/utilities
        src/communication
        
    REQUIRES
        # ESP-IDF components
        driver
        esp_wifi
        esp_bt
        bt
        nvs_flash
        esp_event
        esp_netif
        esp_https_ota
        esp_http_client
        esp_timer
        mbedtls
        lwip
        freertos
        spi_flash
        bootloader_support
        app_update
        log
        
    PRIV_REQUIRES
        # Additional libraries
        # These will be resolved from the build system
)

# ============================================================
# EXTERNAL LIBRARIES
# ============================================================

# Arduino library (if using Arduino framework)
if(CONFIG_ARDUINO_ENABLED)
    target_link_libraries(${COMPONENT_TARGET} INTERFACE arduino)
endif()

# ============================================================
# CUSTOM TARGETS
# ============================================================

# Create a target for generating documentation
add_custom_target(docs
    COMMAND doxygen Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
    COMMENT "Generating documentation with Doxygen"
)

# Create a target for running unit tests
add_custom_target(test
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND esptool.py --chip esp32 --port ${ESPTOOL_PORT} --baud ${ESPTOOL_BAUD} write_flash ${ESPTOOL_FLASH_MODE} ${ESPTOOL_FLASH_FREQ} ${ESPTOOL_FLASH_SIZE} 0x1000 ${CMAKE_BINARY_DIR}/bootloader.bin 0x8000 ${CMAKE_BINARY_DIR}/partition_table.bin 0x10000 ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Running unit tests..."
)

# ============================================================
# PARTITION TABLE
# ============================================================

# Set partition table
if(NOT CONFIG_PARTITION_TABLE_CUSTOM)
    set(CONFIG_PARTITION_TABLE_CUSTOM ${CMAKE_CURRENT_SOURCE_DIR}/partitions.csv)
endif()

# ============================================================
# BOOTLOADER CONFIGURATION
# ============================================================

# Set bootloader configuration
set(BOOTLOADER_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/bootloader)

# ============================================================
# POST-BUILD ACTIONS
# ============================================================

# Generate build information
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "  SmartLock Pro v${PROJECT_VER}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Build: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "  Built: ${CMAKE_CURRENT_TIMESTAMP}"
    COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
)

# Generate binary size report
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
    COMMENT "Firmware size report:"
)

# ============================================================
# BUILD OPTIONS
# ============================================================

# Enable PSRAM if available
set(CONFIG_ESP32_SPIRAM_SUPPORT ON)

# Enable heap poisoning for debugging
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CONFIG_HEAP_POISONING_COMPREHENSIVE ON)
endif()

# Set flash configuration
set(CONFIG_ESPTOOLPY_FLASHSIZE "4MB")
set(CONFIG_ESPTOOLPY_FLASHFREQ "80m")
set(CONFIG_ESPTOOLPY_FLASHMODE "dout")

# Set CPU frequency
set(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ "240")

# Enable watchdog
set(CONFIG_ESP_TASK_WDT ON)
set(CONFIG_ESP_TASK_WDT_TIMEOUT_S "10")

# Enable OTA
set(CONFIG_APP_OTA_UPDATE ON)
set(CONFIG_APP_OTA_UPDATE_WITHOUT_SYSTEM_VERIFY ON)

# ============================================================
# ADDITIONAL COMPONENTS
# ============================================================

# Include the main component
idf_build_process()

# ============================================================
# EXTERNAL LIBRARIES (if not using Arduino)
# ============================================================

# Add external libraries as components
# Uncomment if you have custom components in the components directory

# add_subdirectory(components)

# ============================================================
# VERSION INFORMATION
# ============================================================

# Generate version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.h
)

# ============================================================
# POST-BUILD COPY
# ============================================================

# Copy firmware binary to output directory
add_custom_command(TARGET ${PROJECT_NAME}.bin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
    COMMENT "Copying firmware to firmware.bin"
)

# Copy partition table
add_custom_command(TARGET ${PROJECT_NAME}.bin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/partition_table.bin
        ${CMAKE_CURRENT_BINARY_DIR}/partitions.bin
    COMMENT "Copying partition table"
)

# ============================================================
# CREATE OTA PACKAGE
# ============================================================

add_custom_target(ota_package
    COMMAND ${CMAKE_COMMAND} -E echo "Creating OTA package..."
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/ota_firmware.bin
    COMMAND ${CMAKE_COMMAND} -E echo "OTA package created: ota_firmware.bin"
    DEPENDS ${PROJECT_NAME}.elf
)

# ============================================================
# CREATE FIRMWARE BUNDLE
# ============================================================

add_custom_target(firmware_bundle
    COMMAND ${CMAKE_COMMAND} -E echo "Creating firmware bundle..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/firmware_bundle
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin ${CMAKE_CURRENT_BINARY_DIR}/firmware_bundle/firmware.bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bootloader.bin ${CMAKE_CURRENT_BINARY_DIR}/firmware_bundle/bootloader.bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/partition_table.bin ${CMAKE_CURRENT_BINARY_DIR}/firmware_bundle/partitions.bin
    COMMAND ${CMAKE_COMMAND} -E echo "Firmware bundle created in firmware_bundle/"
)

# ============================================================
# QUICK BUILD TARGETS
# ============================================================

# Flash the firmware
add_custom_target(flash
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target flash
    COMMENT "Flashing firmware..."
)

# Monitor the serial output
add_custom_target(monitor
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target monitor
    COMMENT "Starting serial monitor..."
    USES_TERMINAL
)

# Clean build
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning all build artifacts..."
)

# ============================================================
# DEPENDENCY MANAGEMENT
# ============================================================

# Add dependency on version header
add_dependencies(${PROJECT_NAME}.elf version_header)

# Ensure version header is generated before compilation
add_custom_target(version_header
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include/version.h
)

# ============================================================
# END OF FILE
# ============================================================
