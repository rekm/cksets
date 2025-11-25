#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <clustering/clustering.hpp>
#include <memory/pool_allocator.hpp>
#include <histogram/map_histogram_collection.hpp>

namespace py = pybind11;


// Initial chatgtp hint for exactly mimiking the function signature 
// of ksets,  Accessed 15.11.25 
py::tuple cluster_binding(py::array input,
                          int K,
                          int max_hist,
                          py::object histograms = py::none(),
                          int max_iter = 50)
{
    // ---- Convert numpy to vector<vector<string>> ----
    // also reworked using chatgtp version 5 (Access 15.11.2025), I got seg faults because of 
    // an improper cast to an object pointer and was pointed at the simple line that follows
    std::vector<std::vector<std::string>> rows;
    try {
        rows = input.cast<std::vector<std::vector<std::string>>>();
    } catch (const py::cast_error &) {
        // If cast operation fails, fall back to iterating over each item in list as python object.
        // 
        py::tuple shape = input.attr("shape");
        ssize_t n_rows = shape[0].cast<ssize_t>();
        ssize_t n_cols = shape[1].cast<ssize_t>();
        rows.resize(n_rows);

        for (ssize_t i = 0; i < n_rows; ++i) {
            rows[i].reserve(n_cols);
            for (ssize_t j = 0; j < n_cols; ++j) {
                // Use Python-level indexing (safe for unicode / object arrays) <- ChatGtP Comment
                // Note: Very slow because it calls the python.__getitem__ method for each array item <- my comment
                py::object item = input.attr("__getitem__")(py::make_tuple(i, j));
                rows[i].push_back(item.cast<std::string>());
            }
        }
    }

    // ---- Convert histograms optionally ----
    std::vector<std::vector<int>> hist_cpp;

    if (!histograms.is_none()) {
        hist_cpp = histograms.cast<std::vector<std::vector<int>>>();
    }

    // ---- Call C++ algorithm ----
    //auto hash_policy = ksets::hashing::HashPolicy<ksets::hashing::MurmurHash>{12345}; // <- does not work because its explicit
    auto hash_policy = ksets::hashing::HashPolicy<ksets::hashing::MurmurHash>(ksets::hashing::MurmurHash(12345));
    //auto alloc_policy = ksets::memory::PoolAllocator<std::pair<const std::string,int>>{};
    ksets::histogram::MapHistogramCollection<decltype(hash_policy)> hist_collection(K, hash_policy);//, alloc_policy);
    ksets::ClusterResult r = ksets::cluster_cpp(rows, K, max_hist, hist_collection, max_iter);

    return py::make_tuple(r.labels, r.histograms, r.sumd);
}



PYBIND11_MODULE(_ksets_cpp, m)
{
    m.def("cluster", &cluster_binding,
          py::arg("data"),
          py::arg("number_of_clusters"),
          py::arg("max_hist_length"),
          py::arg("histograms") = py::none(),
          py::arg("max_iter") = 50);
}
