cmake_minimum_required(VERSION 3.27)

project(flujo)

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
    flujo/main.cpp
)

target_include_directories(flujo PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(flujo
    PUBLIC
        TOML_EXCEPTIONS=0
)

add_subdirectory("submodules/kouta")
add_subdirectory("submodules/spdlog")
add_subdirectory("submodules/tomlplusplus")

target_link_libraries(flujo
    PRIVATE
        kouta-base-header
        kouta-io-header
        spdlog
        tomlplusplus::tomlplusplus

)
