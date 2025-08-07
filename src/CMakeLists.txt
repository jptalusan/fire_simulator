# Source files - find all .cpp files recursively except main.cpp
file(GLOB_RECURSE ALL_CPP_FILES "*.cpp")
list(REMOVE_ITEM ALL_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
set(SOURCES ${ALL_CPP_FILES})

# Create a static library for the core functionality
add_library(${PROJECT_NAME}_lib STATIC ${SOURCES})

# Link libraries to the static library
target_link_libraries(${PROJECT_NAME}_lib
    Boost::system
    Boost::filesystem
    Boost::thread
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    CURL::libcurl
    ${LPSOLVE_LIBRARY}
)

# Include directories for the static library
target_include_directories(${PROJECT_NAME}_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Create executable
add_executable(${PROJECT_NAME} main.cpp)

# Link the static library to the executable
target_link_libraries(${PROJECT_NAME}
    ${PROJECT_NAME}_lib
)
