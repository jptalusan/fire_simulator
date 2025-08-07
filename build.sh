#!/bin/bash

# Build script for C++ macOS Boilerplate
# This script automates the build process

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

# Parse command line arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
RUN_TESTS=false
VERBOSE=false
DOXYGEN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -x|--doxygen)
            DOXYGEN=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -d, --debug    Build in Debug mode (default: Release)"
            echo "  -c, --clean    Clean build directory before building"
            echo "  -t, --test     Run tests after building"
            echo "  -v, --verbose  Verbose output"
            echo "  -x, --doxygen  Generate Doxygen documentation"
            echo "  -h, --help     Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "Building C++ macOS Boilerplate in $BUILD_TYPE mode"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    print_status "Cleaning build directory..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
print_status "Configuring with CMake..."
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [ "$VERBOSE" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_VERBOSE_MAKEFILE=ON"
fi
if [ "$DOXYGEN" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_DOCS=ON"
fi

cmake .. $CMAKE_ARGS

# Build the project
print_status "Building project..."
if [ "$VERBOSE" = true ]; then
    make VERBOSE=1 -j$(sysctl -n hw.ncpu)
else
    make -j$(sysctl -n hw.ncpu)
fi

if [ "$DOXYGEN" = true ]; then
    print_status "Generating Doxygen documentation..."
    make docs
fi

print_status "Build completed successfully!"

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    print_status "Running tests..."
    if [ -f "tests/unit_tests" ]; then
        ./tests/unit_tests
        print_status "All tests passed!"
    else
        print_warning "Test executable not found. Tests may not have been built."
    fi
fi

print_status "Done! Executable is located at: build/src/fire_simulator"
