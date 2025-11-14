# SPDX-FileCopyrightText: 2006-2025 Knut Reinert & Freie Universität Berlin
# SPDX-FileCopyrightText: 2016-2025 Knut Reinert & MPI für molekulare Genetik
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required (VERSION 3.25...3.30)

# ----------------------------------------------------------------------------
# Short-circuit if Raptor is already configured
# ----------------------------------------------------------------------------

if (TARGET ksets::interface)
    return ()
endif ()

# ----------------------------------------------------------------------------
# Greeter
# ----------------------------------------------------------------------------

message (STATUS "Finding Ksets (${KSETS_VERSION}) and checking requirements")

# ----------------------------------------------------------------------------
# Pretty printing and error handling
# ----------------------------------------------------------------------------

macro (ksets_config_print text)
    message (STATUS "  ${text}")
endmacro ()

macro (ksets_config_error text)
    message (FATAL_ERROR "  ${text}")
endmacro ()

# ----------------------------------------------------------------------------
# C++ standard
# ----------------------------------------------------------------------------

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set (CMAKE_CXX_STANDARD 23)
endif ()

if (NOT DEFINED CMAKE_CXX_STANDARD_REQUIRED)
    set (CMAKE_CXX_STANDARD_REQUIRED OFF)
endif ()

if (NOT DEFINED CMAKE_CXX_EXTENSIONS)
    set (CMAKE_CXX_EXTENSIONS OFF)
endif ()

# ----------------------------------------------------------------------------
# LTO
# ----------------------------------------------------------------------------

include (CheckIPOSupported)
check_ipo_supported (RESULT KSETS_HAS_LTO OUTPUT KSETS_HAS_LTO_OUTPUT)
if (KSETS_HAS_LTO)
    set (CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

# ----------------------------------------------------------------------------
# CPM
# ----------------------------------------------------------------------------

set (CPM_INDENT "  CMake Package Manager CPM: ")
include (${KSETS_SOURCE_DIR}/cmake/CPM.cmake)
CPMUsePackageLock (${KSETS_SOURCE_DIR}/cmake/package-lock.cmake)


# ----------------------------------------------------------------------------
# Public include path
# ----------------------------------------------------------------------------


find_path(KSETS_INCLUDE_DIR
          NAMES clustering/ksets.hpp clustering/clustering.hpp version.hpp
          HINTS "${PROJECT_SOURCE_DIR}/include"
)

if (KSETS_INCLUDE_DIR)
    ksets_config_print("Found include directory: ${KSETS_INCLUDE_DIR}")
else()
    ksets_config_error("Could not locate include directory")
endif()



# ----------------------------------------------------------------------------
# Find or add dependencies
# ----------------------------------------------------------------------------

CPMGetPackage (use_ccache)
CPMGetPackage (hibf)
CPMGetPackage (sharg)
CPMGetPackage (seqan3)
CPMGetPackage (pybind11)

# Only include Chopper if your project uses HIBF layout computation
option(KSETS_USE_CHOPPER "Enable chopper dependency" OFF)
if(KSETS_USE_CHOPPER)
    CPMGetPackage(chopper)
endif()

# ----------------------------------------------------------------------------
# Interface target
# ----------------------------------------------------------------------------

add_library(ksets_interface INTERFACE)

target_include_directories(ksets_interface
    INTERFACE
        "${KSETS_INCLUDE_DIR}"
)

target_link_libraries(ksets_interface
    INTERFACE
        seqan3::seqan3
        hibf::hibf
        sharg::sharg
)

# If Chopper is enabled:
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

add_library(ksets::interface ALIAS ksets_interface)
