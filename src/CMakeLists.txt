# Source files - find all .cpp files recursively except main.cpp
file(GLOB_RECURSE ALL_CPP_FILES "*.cpp")
list(REMOVE_ITEM ALL_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
set(SOURCES ${ALL_CPP_FILES})

# Create a static library for the core functionality
add_library(${PROJECT_NAME}_lib STATIC ${SOURCES})

# Link libraries to the static library
target_link_libraries(${PROJECT_NAME}_lib
    Boost::regex
    Boost::filesystem
    Boost::thread
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    CURL::libcurl
    ${LPSOLVE_LIBRARY}
)

# Use custom-built ONNX Runtime (installed locally)
# Make sure you have built onnxruntime and placed it in a directory below.
set(ONNXRUNTIME_INSTALL_DIR "${PROJECT_SOURCE_DIR}/external/onnxruntime_install/usr/local")
set(ONNXRUNTIME_LIB "${ONNXRUNTIME_INSTALL_DIR}/lib/libonnxruntime.dylib")

# Include directories for the static library
target_include_directories(${PROJECT_NAME}_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${ONNXRUNTIME_INSTALL_DIR}/include
)

# Create executable
add_executable(${PROJECT_NAME} main.cpp)

# Link the static library to the executable
target_link_libraries(${PROJECT_NAME}
    ${PROJECT_NAME}_lib
    ${ONNXRUNTIME_LIB}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "${ONNXRUNTIME_INSTALL_DIR}/lib"
)