cmake_minimum_required(VERSION 3.10)

# Output program names
set(PROGRAM_NAME "run_simulator")
set(TEST_NAME "test_simulator")

# Set the project name and version
project(${PROGRAM_NAME} VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# For GoogleTest test cases
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Find external packages
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add external include directories
include_directories(${CURL_INCLUDE_DIRS})

# Add all source files in the src/ directory
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE TESTSOURCES "test/*.cpp")

# Add sources to the executable
add_executable(${PROGRAM_NAME} ${SOURCES})

# For testing
add_executable(${TEST_NAME} ${TESTSOURCES})
add_test(NAME RunUnitTests COMMAND ${TEST_NAME})

# Linking external libraries
target_link_libraries(${TEST_NAME} GTest::gtest_main pthread)
target_link_libraries(${PROGRAM_NAME} PRIVATE CURL::libcurl)
target_link_libraries(${PROGRAM_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROGRAM_NAME} PRIVATE spdlog::spdlog)

# Set different compile definitions and options for debug and release builds
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${PROGRAM_NAME} PRIVATE DEBUG=1)
    target_compile_options(${PROGRAM_NAME} PRIVATE -g -Wall -Wextra)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(${PROGRAM_NAME} PRIVATE NDEBUG=1)
    target_compile_options(${PROGRAM_NAME} PRIVATE -O2 -Wall -Wextra)
endif()

# Set the output directory for the binary (These are optional)
set_target_properties(${PROGRAM_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# Set the output directory for the test binary (debug)
set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# Optional: install target (if you want to make use of `make install`)
install(TARGETS ${PROGRAM_NAME} DESTINATION ${PROJECT_SOURCE_DIR}/binaries)

