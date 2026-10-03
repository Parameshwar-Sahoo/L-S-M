cmake_minimum_required(VERSION 3.16)
project(SmartLMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -O2)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE HAL_SOURCES  "src/hal/*.cpp")
file(GLOB_RECURSE IPC_SOURCES  "src/ipc/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")
file(GLOB_RECURSE UI_SOURCES   "src/ui/*.cpp")

set(COMMON_SOURCES
    ${CORE_SOURCES}
    ${HAL_SOURCES}
    ${IPC_SOURCES}
    ${UTILS_SOURCES}
)

find_package(Threads REQUIRED)

# Main Executable
add_executable(library_system
    ${COMMON_SOURCES}
    ${UI_SOURCES}
    src/main.cpp
)
target_link_libraries(library_system PRIVATE Threads::Threads rt)

# Test Suite Executable
add_executable(test_runner
    ${COMMON_SOURCES}
    tests/test_runner.cpp
)
target_link_libraries(test_runner PRIVATE Threads::Threads rt)

enable_testing()
add_test(NAME AllUnitAndIntegrationTests COMMAND test_runner)
