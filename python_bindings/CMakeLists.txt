pybind11_add_module(clustering_cpp bindings.cpp)

target_link_libraries(clustering_cpp
    PRIVATE clustering_core
)

target_include_directories(clustering_cpp PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)