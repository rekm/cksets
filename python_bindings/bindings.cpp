#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <clustering/clustering.hpp>

namespace py = pybind11;


// Initial chatgtp hint for exactly mimicing the function signature 
// of ksets,  Accessed 15.11.25 
py::tuple cluster_binding(py::array input,
                          int K,
                          int max_hist,
                          py::object histograms = py::none(),
                          int max_iter = 50)
{
    // ---- Convert numpy → vector<vector<string>> ----
    py::array arr = input;
    py::buffer_info info = arr.request();

    auto data_ptr = static_cast<py::object*>(info.ptr);
    std::vector<std::vector<std::string>> rows;

    rows.resize(info.shape[0]);

    size_t ncols = info.shape[1];

    for (ssize_t i = 0; i < info.shape[0]; i++) {
        rows[i].reserve(ncols);
        for (ssize_t j = 0; j < ncols; j++) {
            py::object item = data_ptr[i * ncols + j];
            rows[i].push_back(item.cast<std::string>());
        }
    }


    // ---- Convert histograms optionally ----
    std::vector<std::vector<int>> hist_cpp;

    if (!histograms.is_none()) {
        hist_cpp = histograms.cast<std::vector<std::vector<int>>>();
    }

    // ---- Call C++ algorithm ----
    ksets::ClusterResult r = ksets::cluster_cpp(rows, K, max_hist, hist_cpp, max_iter);

    return py::make_tuple(r.labels, r.histograms, r.sumd);
}

PYBIND11_MODULE(ksets_cpp, m)
{
    m.def("cluster", &cluster_binding,
          py::arg("data"),
          py::arg("number_of_clusters"),
          py::arg("max_hist_length"),
          py::arg("histograms") = py::none(),
          py::arg("max_iter") = 50);
}
