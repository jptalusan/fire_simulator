include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Verify test sources exist
if(NOT TEST_SOURCES)
    message(WARNING "No test source files found in ${CMAKE_CURRENT_SOURCE_DIR}")
    return()
endif()

enable_testing()

# Create test executable
add_executable(unit_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(unit_tests
    ${PROJECT_NAME}_lib
    GTest::gmock
)

# Include directories
target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Add tests to CTest
add_test(NAME unit_tests COMMAND unit_tests)

# Enable GoogleTest discovery
include(GoogleTest)
gtest_discover_tests(unit_tests)
