# pybind11 module
pybind11_add_module(ksets_cpp bindings.cpp)

# Link against your core library
target_link_libraries(ksets_cpp
    PRIVATE
        ksets::ksets            # or ksets::ksets
)

# Make sure headers work
target_include_directories(ksets_cpp
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
