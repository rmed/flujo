cmake_minimum_required(VERSION 3.27)

project(flujo)

# Dependencies
add_subdirectory("submodules/jsonrpcpp")
add_subdirectory("submodules/kouta")
add_subdirectory("submodules/spdlog")
add_subdirectory("submodules/tomlplusplus")
add_subdirectory("submodules/tgbot-cpp")

# Flujo config
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

include_directories("flujo")

# Configuration options
set(FLUJO_DEFAULT_CONFIG "/etc/flujo/config.toml" CACHE PATH "default path in which to look for application configuration")

configure_file("flujo/compile-detail.h.in" "compile-detail.h")

# Define target
add_executable(flujo
    flujo/config/loader.cpp
    flujo/server/comms/telegram.cpp
    flujo/server/dispatcher.cpp
    flujo/server/server.cpp
    flujo/server/session.cpp
    flujo/main.cpp
)

target_include_directories(flujo PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(flujo
    PUBLIC
        TOML_EXCEPTIONS=0
)

target_include_directories(flujo PRIVATE "submodules/jsonrpcpp/include")

target_link_libraries(flujo
    PRIVATE
        kouta-base-header
        kouta-io-header
        spdlog
        tomlplusplus::tomlplusplus
        TgBot
)
