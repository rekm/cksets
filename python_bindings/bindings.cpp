#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <ksets/hashing/murmur_hash.hpp>
#include <ksets/clustering/clustering.hpp>
#include <ksets/memory/pool_allocator.hpp>
#include <ksets/histogram/collection_map.hpp>
#include <ksets/clustering/histogram_update_memorize_changes.hpp>
#include <ksets/clustering/histogram_update_ksets_authors.hpp>
#include <ksets/clustering/histogram_preselect_nothing.hpp>
#include <ksets/clustering/histogram_preselect_ibf.hpp>
#include <ksets/token_encoder/reversible.hpp>
#include <ksets/types.hpp>
#include <ksets/telemetry.hpp>

namespace py = pybind11;


// This is bad but should be alright for now 
// Needs to be properly adressed and itegrated into python 
struct CerrSink {
    // Maximum telemetry level this sink will handle
    static constexpr ksets::telemetry::TelemetryLevel level =
        ksets::telemetry::TelemetryLevel::basic;

    template<class Event>
    void handle(const Event& e) const {
        print(e);
    }

private:

    // Generic fallback (if no specific overload exists)
    template<class Event>
    void print(const Event& e) const {
    }


    void print(const ksets::AlgorithmStart& e ) const {
        std::cerr << "[Algorithm Initialized]\n" 
                  << "Histograms Provided: " << (e.histograms_provided ? "TRUE" : "FALSE") 
                  << "\nData Size: " << e.n_items << "\n";
    }

    // IterationMetrics specialization
    void print(const ksets::IterationMetrics& e) const {
        
        std::cerr << "[Iteration " << e.iter << "] "
                  << "Objective: " << e.objective
                  << ", Delta: " << e.delta;
        std::cerr << "\n"; 
    }

    // RunSummary example
    void print(const ksets::RunSummary& e) const {
        std::cerr << "[ Final Summary ]\n";     
        std::cerr << "Total Iterations: " << e.iterations << "\n"
                << "Avg Histogram Size: " << e.hist.avg << "\n"
                << "Median Histogram Size: " << e.hist.median << "\n"
                << "Sum of Total Distances: " << e.sum_distances << "\n"; 
    }
};


//Get String Object Matrix from python 
template<class Encoder>
std::vector<std::vector<ksets::token_t>> encode_from_python(
            py::handle data, 
            Encoder& encoder
        ){
    std::vector<std::vector<ksets::token_t>> out;

    py::iterable rows = py::reinterpret_borrow<py::iterable>(data);

    for (py::handle row_obj : rows) {
        py::iterable row = py::reinterpret_borrow<py::iterable>(row_obj);
        
        std::vector<ksets::token_t> encoded_row;
        encoded_row.reserve(py::len(row));
        for (py::handle item : row) { 
            if (py::isinstance<py::int_>(item)) {
                encoded_row.push_back(item.cast<ksets::token_t>());
            } else {
                // zero-copy view into Python string
                
                py::str s = py::reinterpret_borrow<py::str>(item);
                Py_ssize_t size;
                const char* ptr = PyUnicode_AsUTF8AndSize(s.ptr(), &size);
                if (!ptr)
                    throw std::runtime_error("Invalid UTF-8");
                encoded_row.push_back(encoder.encode(std::string_view(ptr, size)));
            }
        }

        out.push_back(std::move(encoded_row));
    }

    return out;
}

// Histogram initialization
template<class Encoder, class HistogramCollection>
void init_histograms_from_python(
    py::object histograms,
    HistogramCollection& hist_collection,
    Encoder& encoder
) {
    if (histograms.is_none())
        return;

    py::list py_list = histograms.cast<py::list>();
    if (py_list.size() != hist_collection.size())
        throw std::runtime_error("Histogram list length != K");

    for (size_t c = 0; c < hist_collection.size(); ++c) {
        py::dict d = py_list[c].cast<py::dict>();
        ksets::token_t tok;
        for (auto kv : d) {
            if (py::isinstance<py::int_>(kv.first)) {
                tok = kv.first.cast<ksets::token_t>();
            } else {
                py::str s = py::reinterpret_borrow<py::str>(kv.first);

                Py_ssize_t size;
                const char* ptr = PyUnicode_AsUTF8AndSize(s.ptr(), &size);
                if (!ptr)
                    throw std::runtime_error("Invalid UTF-8");

                tok = encoder.encode(std::string_view(ptr, size));         
            }
            int count = kv.second.cast<int>();
            hist_collection[c].set(tok, count);
        }
    }
}



// Initial chatgtp hint for exactly mimicking the function signature 
// of ksets,  Accessed 15.11.25 (Version 5.1)
py::tuple cluster_binding(py::array input,
                          int K,
                          int max_hist,
                          py::object histograms = py::none(),
                          int max_iter = 50,
                          std::string hist_rebuild = "memorize", 
                          std::string preselect = "none",
                          int seed = 12345)
{
    // ---- Convert numpy to vector<vector<string>> ----
    // also reworked using chatgtp version 5 (Access 15.11.2025), I got seg faults because of 
    // an improper cast to an object pointer and was pointed at the simple line that follows
    std::vector<std::vector<ksets::token_t>> rows;
    ksets::token_encoder::ReversibleInterning encoder;
    
    rows = encode_from_python(input, encoder);


    // ---- Call C++ algorithm ----

    //auto hash_policy = ksets::hashing::HashPolicy<ksets::hashing::MurmurHash>{12345}; // <- does not work because its explicit
    auto hash_policy = ksets::hashing::HashPolicy<ksets::hashing::MurmurHash>(ksets::hashing::MurmurHash(seed));
    //auto alloc_policy = ksets::memory::PoolAllocator<std::pair<const std::string,int>>{}; <- crashes because the PoolAllocator has little human brain power supportiing it
    ksets::histogram::MapHistogramCollection<decltype(hash_policy)> hist_collection(K, hash_policy);//, alloc_policy);
    init_histograms_from_python(histograms, hist_collection, encoder);
    CerrSink cerr_sink{};
    ksets::ClusterResult r;
    if (hist_rebuild == "memorize") {
        if (preselect == "none") {
            r = r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterMemorizeChanges,
                ksets::HistPreselectNothing
                >(rows, K, max_hist, hist_collection, max_iter,seed, cerr_sink); 
        } else if (preselect == "ibf") {
            r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterMemorizeChanges,
                ksets::HistPreselectIBF
                >(rows, K, max_hist, hist_collection, max_iter,seed, cerr_sink);
        }
    } else if (hist_rebuild == "full"){
        if (preselect == "none") {
            r = r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterKsetsAuthors,
                ksets::HistPreselectNothing
                >(rows, K, max_hist, hist_collection, max_iter,seed, cerr_sink); 
        } else if (preselect == "ibf") {
            r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterKsetsAuthors,
                ksets::HistPreselectIBF
                >(rows, K, max_hist, hist_collection, max_iter,seed, cerr_sink);
        }
    } else {
        throw std::runtime_error("Unknown hist_rebuild option: " + hist_rebuild +
                             ". Expected: 'memorize' or 'full'");
    }

    return py::make_tuple(r.labels, py::none(), r.objective);
}



PYBIND11_MODULE(_ksets_cpp, m)
{
    m.def("cluster", &cluster_binding,
          py::arg("data"),
          py::arg("number_of_clusters"),
          py::arg("max_hist_length"),
          py::arg("histograms") = py::none(),
          py::arg("max_iter") = 50,
          py::arg("hist_rebuild") = "memorize",
          py::arg("preselect") = "none",
          py::arg("seed") = 12345
        );
}
