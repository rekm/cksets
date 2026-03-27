

# ----------------------------------------------------------------------------
# CPM
# ----------------------------------------------------------------------------

set (CPM_INDENT "  CMake Package Manager CPM: ")
include (${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
CPMUsePackageLock (${CMAKE_CURRENT_LIST_DIR}/package-lock.cmake)


# ----------------------------------------------------------------------------
# Find or add dependencies
# ----------------------------------------------------------------------------

CPMGetPackage (use_ccache)
CPMGetPackage (hibf)
CPMGetPackage (sharg)
CPMGetPackage (seqan3)
CPMGetPackage (pybind11)
# Disable install/config generation for dependencies
set(TSL_ROBIN_MAP_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
CPMGetPackage (robinmap)
CPMGetPackage (gtl)

# Only include Chopper if your project uses HIBF layout computation
option(KSETS_USE_CHOPPER "Enable chopper dependency" OFF)
if(KSETS_USE_CHOPPER)
    CPMGetPackage(chopper)
endif()