cmake_minimum_required(VERSION 3.25...3.30)
project(KSets VERSION 0.1.0 LANGUAGES CXX)

# ---------------------------------------
# Global settings
# ---------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CPM_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/external")


# ---------------------------------------
# CPM Package Manager
# ---------------------------------------
set(CPM_DOWNLOAD_LOCATION "${CMAKE_SOURCE_DIR}/external/CPM.cmake")
include(${CPM_DOWNLOAD_LOCATION})


# ---------------------------------------
# Core library
# ---------------------------------------
add_library(ksets
    src/clustering.cpp
    src/clustering/ksets.cpp
    src/indexing/ibf_processor.cpp
    src/hashing/MurmurHash3.cpp
)

target_include_directories(ksets
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ksets
    PUBLIC
        ksets::interface   # brings seqan3 + hibf + sharg
)

add_library(ksets::ksets ALIAS ksets)

# ---------------------------------------
# Executables (main programs)
# ---------------------------------------
add_executable(cksets src/main/ksets.cpp)
target_link_libraries(cksets PRIVATE ksets)

# ---------------------------------------
# Subdirectories
# ---------------------------------------
add_subdirectory(benchmarks)
enable_testing()
add_subdirectory(tests)

# ---------------------------------------
# Python Bindings
# ---------------------------------------
option(BUILD_PYTHON_BINDINGS "Build pybind11 extension" ON)
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python_bindings)
endif()

# ---------------------------------------
# Installation (optional but recommended)
# ---------------------------------------
install(
    TARGETS ksets
    EXPORT ksetsTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

install(
    EXPORT ksetsTargets
    FILE KSetsTargets.cmake
    NAMESPACE ksets::
    DESTINATION lib/cmake/ksets
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/KSetsConfig.cmake.in
    ${CMAKE_BINARY_DIR}/KSetsConfig.cmake
    INSTALL_DESTINATION lib/cmake/ksets
)

install(FILES ${CMAKE_BINARY_DIR}/KSetsConfig.cmake DESTINATION lib/cmake/ksets)
