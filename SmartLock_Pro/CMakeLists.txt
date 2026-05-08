cmake_minimum_required(VERSION 3.15)
project(SmartLockPro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common library
add_library(common STATIC
    common/encryption.cpp
    common/sha256.c
    common/base64.c
)

target_include_directories(common PUBLIC common/include)

# Desktop controller
add_executable(smartlock_controller
    desktop_controller/main.cpp
    desktop_controller/webserver.cpp
    desktop_controller/database.cpp
    desktop_controller/auth_manager.cpp
    desktop_controller/access_logger.cpp
    desktop_controller/websocket_server.cpp
)

target_link_libraries(smartlock_controller
    common
    sqlite3
    pthread
    websockets
)

target_include_directories(smartlock_controller PRIVATE
    desktop_controller/include
    common/include
)

# ESP32 firmware (requires platformio or arduino build system)
# This is handled separately with PlatformIO

# Installation
install(TARGETS smartlock_controller DESTINATION bin)
install(FILES README.md DESTINATION .)
