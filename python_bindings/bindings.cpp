#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <clustering/clustering.hpp>
#include <clustering/algorithm_selector.hpp>

namespace py = pybind11;

PYBIND11_MODULE(clustering_cpp, m) {
    m.doc() = "Python bindings for clustering project";

    m.def("cluster",
          [](const std::vector<std::string>& sequences, std::string algo) {
              auto selected = clustering::select_algorithm(algo);
              return selected(sequences);
          });
}
