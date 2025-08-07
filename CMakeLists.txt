cmake_minimum_required(VERSION 3.16)
project(fire_simulator 
    VERSION 1.0
    DESCRIPTION "A digital twin for fire dispatch"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required dependencies
cmake_policy(SET CMP0167 OLD)
find_package(Boost REQUIRED COMPONENTS filesystem thread)
# find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)

# Get the include directory from the imported target
get_target_property(SPDLOG_INCLUDE_DIR spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)

# Check if the header exists in the include directory
if(EXISTS "${SPDLOG_INCLUDE_DIR}/spdlog/stopwatch.h")
    add_definitions(-DHAVE_SPDLOG_STOPWATCH)
endif()

# Find lp_solve
find_path(LPSOLVE_INCLUDE_DIR lp_lib.h)
find_library(LPSOLVE_LIBRARY lpsolve55)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIR}
    ${LPSOLVE_INCLUDE_DIR}
)

# Add subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND AND BUILD_DOCS)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
