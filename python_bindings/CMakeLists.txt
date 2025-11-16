cmake_minimum_required(VERSION 3.25...3.30)

find_package(pybind11 REQUIRED)
# pybind11 module
pybind11_add_module(_ksets_cpp bindings.cpp)

# Link against your core library
target_link_libraries(_ksets_cpp
    PRIVATE
        ksets::ksets            # or ksets::ksets
)

# Make sure headers work
target_include_directories(_ksets_cpp
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

# Install compiled module into the Python package
install(TARGETS _ksets_cpp
    LIBRARY DESTINATION ${Python3_SITEARCH}/ksets_cpp  # Linux/macOS
    RUNTIME DESTINATION ${Python3_SITEARCH}/ksets_cpp  # Windows
)

# Install helpers and __init__.py
#install(FILES ${CMAKE_SOURCE_DIR}/python/helpers.py
#              ${CMAKE_SOURCE_DIR}/python/__init__.py
#        DESTINATION ${Python3_SITEARCH}/ksets_cpp)
