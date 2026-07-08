# SPDX-FileCopyrightText: 2006-2025 Knut Reinert & Freie Universität Berlin
# SPDX-FileCopyrightText: 2016-2025 Knut Reinert & MPI für molekulare Genetik
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required (VERSION 3.25...3.30)

# ----------------------------------------------------------------------------
# Short-circuit if Ksets is already configured
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
# Public include path
# ----------------------------------------------------------------------------


find_path(KSETS_INCLUDE_DIR
          NAMES ksets.hpp clustering/clustering.hpp version.hpp
          HINTS "${PROJECT_SOURCE_DIR}/include"
)

if (KSETS_INCLUDE_DIR)
    ksets_config_print("Found include directory: ${KSETS_INCLUDE_DIR}")
else()
    ksets_config_error("Could not locate include directory")
endif()





# ----------------------------------------------------------------------------
# Interface target
# ----------------------------------------------------------------------------

#add_library(ksets_interface INTERFACE)
#
#target_include_directories(ksets_interface
#    INTERFACE
#        $<BUILD_INTERFACE:${KSETS_INCLUDE_DIR}>
#        $<INSTALL_INTERFACE:include>
#)

#target_link_libraries(ksets_interface
#    INTERFACE
#        $<BUILD_INTERFACE:seqan3::seqan3>
#        $<BUILD_INTERFACE:seqan::hibf>
#        $<BUILD_INTERFACE:sharg::sharg>
#        $<BUILD_INTERFACE:tsl::robin_map>

#        $<INSTALL_INTERFACE:seqan3::seqan3>
#        $<INSTALL_INTERFACE:seqan::hibf>
#        $<INSTALL_INTERFACE:sharg::sharg>
#        $<INSTALL_INTERFACE:tsl::robin_map>
#)

# If Chopper is enabled:


#add_library(ksets::interface ALIAS ksets_interface)
