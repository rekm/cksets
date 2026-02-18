cmake_minimum_required(VERSION 3.25...3.30)
project(KSets VERSION 0.1.0 LANGUAGES CXX)

# ---------------------------------------
# Global settings
# ---------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(CPM_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/external")
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/external") # That is close to what I want to achieve
set(Python3_EXECUTABLE "" CACHE FILEPATH "Path to Python interpreter")
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# ---------------------------------------
# CPM Package Manager
# ---------------------------------------
set(CPM_DOWNLOAD_LOCATION "${CMAKE_SOURCE_DIR}/external/CPM.cmake")
#include(${CPM_DOWNLOAD_LOCATION})


# ---------------------------------------
# Core library
# ---------------------------------------

    


if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include(${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/configuration.cmake)

add_library(ksets_interface INTERFACE)
set_target_properties(ksets_interface PROPERTIES EXPORT_NAME interface)

target_include_directories(ksets_interface
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ksets_interface
    INTERFACE
        # build-tree dependencies
        $<BUILD_INTERFACE:seqan3::seqan3>
        $<BUILD_INTERFACE:seqan::hibf>
        $<BUILD_INTERFACE:sharg::sharg>
        $<BUILD_INTERFACE:tsl::robin_map>
#
#        # install-tree dependencies
#        $<INSTALL_INTERFACE:seqan3::seqan3>
#        $<INSTALL_INTERFACE:seqan::hibf>
#        $<INSTALL_INTERFACE:sharg::sharg>
#        $<INSTALL_INTERFACE:tsl::robin_map>
)
# Build-tree linkage (CPM targets)



add_library(ksets::interface ALIAS ksets_interface)

if(KSETS_USE_CHOPPER)
    target_link_libraries(ksets_interface INTERFACE chopper::interface)
endif()

# ----------------------------------------------------------------------------
# Optional: Export seqan3 test utilities (only if needed)
# ----------------------------------------------------------------------------

option(KSETS_USE_SEQAN3_TEST_HEADERS "Expose SeqAn3 test utils" OFF)

if(KSETS_USE_SEQAN3_TEST_HEADERS)
    find_path(SEQAN3_TEST_INCLUDE_DIR
              NAMES seqan3/test/tmp_directory.hpp
              HINTS "${seqan3_SOURCE_DIR}/test/include"
    )
    target_include_directories(ksets_interface SYSTEM INTERFACE "${SEQAN3_TEST_INCLUDE_DIR}")
    ksets_config_print("Added seqan3 test headers: ${SEQAN3_TEST_INCLUDE_DIR}")
endif()




add_library(ksets
    src/ksets/clustering.cpp
    src/ksets/clustering/ksets.cpp
    src/ksets/indexing/ibf_processor.cpp
    src/ksets/hashing/MurmurHash3.cpp
)
set_target_properties(ksets PROPERTIES EXPORT_NAME ksets)

target_include_directories(ksets
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
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
target_link_libraries(cksets PRIVATE ksets sharg::sharg seqan::hibf tsl::robin_map)
target_compile_options(cksets PRIVATE -g -fno-omit-frame-pointer)


add_executable(genicds src/scripts/generate_icds.cpp)
target_include_directories(genicds
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(genicds PRIVATE sharg::sharg)

# ---------------------------------------
# Subdirectories
# ---------------------------------------
add_subdirectory(benchmarks)
enable_testing()
#add_subdirectory(tests)

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
    TARGETS 
    ksets 
    ksets_interface
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
