#include <iostream>
#include <string>
#include <sharg/parser.hpp>
#include <sharg/validators.hpp>
#include <ksets/utils/csv.hpp>
#include <ksets/histogram/collection_map.hpp>
#include <ksets/hashing/murmur_hash.hpp>
#include <ksets/token_encoder/reversible.hpp>
#include <ksets/clustering/clustering.hpp>
#include <ksets/clustering/histogram_update_memorize_changes.hpp>
#include <ksets/clustering/histogram_update_ksets_authors.hpp>
#include <ksets/clustering/histogram_preselect_nothing.hpp>
#include <ksets/clustering/histogram_preselect_ibf.hpp>
#include <ksets/types.hpp>
#include <ksets/telemetry.hpp>




void parse_data(
        csv::CSVReader & reader, 
        std::vector<std::vector<ksets::token_t>> & data, 
        ksets::token_encoder::ReversibleInterning &encoder){
    for (csv::CSVRow& row: reader) { // Input iterator
        std::vector<ksets::token_t> encoded_row;
        for (csv::CSVField& field: row) {
            // By default, get<>() produces a std::string.
            // A more efficient get<string_view>() is also available, where the resulting
            // string_view is valid as long as the parent CSVRow is alive
            encoded_row.push_back(encoder.encode(field.get<>()));
        }
        data.push_back(encoded_row);
    }

} 

struct CerrSink {
    // Maximum telemetry level this sink will handle
    static constexpr ksets::telemetry::TelemetryLevel level =
        ksets::telemetry::TelemetryLevel::debug;

    template<class Event>
    void handle(const Event& e) const {
        print(e);
    }

private:
    mutable size_t savings_events = 0;
    mutable double savings_preselect = 0.0;

    // Generic fallback (if no specific overload exists)
    template<class Event>
    void print(const Event& e) const {
        std::cerr << "[Telemetry] Event: " << typeid(e).name() << "\n";
    }

    void print(const ksets::PreselectSavingsTelemetry& e) const {
        savings_preselect += 1.0 - double(e.total_hists_touched) / double(e.total_tokens * e.K);
        savings_events++;
          
    }

    void print(const ksets::AlgorithmStart& e ) const {
        std::cerr << "[Algorithm Initialized]\n" 
                  << "Histograms Provided: " << (e.histograms_provided ? "TRUE" : "FALSE") 
                  << "\nData Size: " << e.n_items << "\n";
    }

    void print(const ksets::DistanceMeasureTelemetry& e) const {
        std::cerr << "[Iteration " << e.iter << ", Data Row: " << e.data_row << " ]" ;
        size_t iter = 0;
        char* sep = " "; 
        for ( const auto &dist: e.distances) {
            std::cerr << sep << iter << ": " << dist;
            ++iter;
            sep = ", ";
        }
        std::cerr <<  "\n";
    }

    void print(const ksets::LabelSelectTelemetry& e) const {
        std::cerr << "[Iteration " << e.iter << ", Data Row: " << e.data_row << " ] " 
                 << "Dist( " << e.old_label << " ) = " << e.old_dist << " --> "
                 << "Dist( " << e.new_label << " ) = " << e.new_dist << "\n";
    }

    // IterationMetrics specialization
    void print(const ksets::IterationMetrics& e) const {
        
        std::cerr << "[Iteration " << e.iter << "] "
                  << "Objective: " << e.objective
                  << ", Delta: " << e.delta;
        if (savings_events > 0){
            double avg_savings = savings_preselect / double{savings_events};
            std::cerr << ", Avg Preselect Savings: " << avg_savings * 100 << "%";  
        }          
        savings_events = 0;
        savings_preselect = 0.0;
        std::cerr << "\n"; 
    }

    // RunSummary example
    void print(const ksets::RunSummary& e) const {
        std::cerr << "[ Final Summary ]\n";     
        std::cerr << "Total Iterations: " << e.iterations << "\n"
                << "Avg Histogram Size: " << e.hist.avg << "\n"
                << "Median Histogram Size: " << e.hist.median << "\n"
                << "Sum of Total Distances: " << e.sum_distances << "\n"; 
        /*size_t hist_quant = 1;
        double hist_quant_perc = 0; 
        for (const auto& chunk : hist_lengths | std::views::chunk(std::max(num_hists / 10, size_t{1}) )) {
            auto chunk_avg = std::reduce(chunk.begin(), chunk.end()) / chunk.size();
            double chunk_perc = chunk.size()/ double{num_hists};
            std::cout << "AVG Histogram Size N-Tile " << hist_quant 
                    << "(n=" << chunk.size() << ", " << hist_quant_perc << "-" << std::min(hist_quant_perc + chunk_perc, 1.0) * 100 
                    << "%) = " << chunk_avg << "\n";
            hist_quant_perc += chunk_perc;
            hist_quant++;  
        }
            std::cerr << "[Run Summary] "
                    << "Iterations: " << e.iterations
                    << ", AvgHist: " << e.avg_hist
                    << ", MedianHist: " << e.median_hist
                    << ", SumDistances: " << e.sum_distances
                    << "\n";*/
    }
};


// Stub was generated using ChatGTP and the following prompt
/*
 I want to have an executable stub ksets.cpp this is the inspiration:
  def cluster(data, number_of_cluster, max_hist_length, histograms = [], max_iter = 0): if max_iter == 0: max_iter = 50 .... 
  i want a small sharg parser that accepts a file as data and int number of clusters and optional max_hist_length and max_iter
*/ 

int main(int argc, char ** argv)
{
    using clock = std::chrono::high_resolution_clock;
    std::filesystem::path data_file{};
    std::string hist_rebuild = "full";
    std::string preselect = "none";
    int number_of_clusters{5};
    int max_hist_length{0};  // default value
    int max_iter{50};          // default value
    int seed{12345};
    bool do_nothing{false};
    try
    {
        // ---------------------------
        // Define CLI parser
        // ---------------------------
        sharg::parser parser{"ksets", argc, argv, sharg::update_notifications::off};
        
        parser.info.short_description = "Ksets clustering algorithm demonstrator";
        parser.info.version = "0.1.0";
        parser.add_option(data_file, sharg::config{.short_id = 'f', .long_id = "file", .description = "Input File" , .validator = sharg::input_file_validator{} });
        parser.add_option(number_of_clusters, sharg::config{
                .short_id = 'k', .long_id = "clusters", 
                .description = "Number of Clusters"});
        //parser.add_option(max_hist_length, sharg::config{.short_id = 'm', .long_id = "max-hist", .description = "Maximum Histogram length"});
        parser.add_option(max_iter, sharg::config{
                .short_id = 'i', .long_id = "max-iter", 
                .description = "Maximum number of iterations (optional)"});
        parser.add_option(preselect, sharg::config{
                .short_id = 'p', .long_id = "preselect",
                .description="Pre-selection strategy for speeding up distance comparisons (ibf,*none) * is default", 
                .validator = sharg::value_list_validator{"none", "ibf"}});
        parser.add_option(hist_rebuild, sharg::config{
                .short_id = 'r', .long_id = "rebuild",
                .description="Rebuild strategy for histograms (*full, memorize) * is default", 
                .validator = sharg::value_list_validator{"full", "memorize"}});
        parser.add_option(seed, sharg::config{
                .short_id = 's', .long_id = "seed", 
                .description = "Seed (optional, default=42)"});
        parser.add_flag(do_nothing, sharg::config{.short_id = 'n', .long_id = "do_nothing", .description = "Only runs file IO"});
        parser.parse();

        // ---------------------------
        // Print values for testing
        // ---------------------------
        std::cerr << "Data file: " << data_file << "\n";
        std::cerr << "Number of clusters: " << number_of_clusters << "\n";
        std::cerr << "Max histogram length: " << max_hist_length << "\n";
        std::cerr << "Max iterations: " << max_iter << "\n";
    }
    catch (sharg::parser_error const & ext) // catch user errors
    {
        std::cerr << "Parsing error: " << ext.what() << "\n";
        return -1;
    }
    // parse data input
    csv::CSVFormat format;
        format.delimiter(' ')
            .trim({'\t'})
            .no_header().quote(false);
    CerrSink cerr_sink{};
    ksets::token_encoder::ReversibleInterning encoder;    
    csv::CSVReader reader(data_file.string(), format); 
    std::vector<std::vector<ksets::token_t>> data;
    auto start = clock::now();
    parse_data(reader, data, encoder);
    auto stop = clock::now();
    double elapsed = std::chrono::duration<double>(stop - start).count();
    std::cerr << " ------ Data -------\nData Rows: " << data.size() << "\n" 
              << "Initial Data Processing (seconds): " << elapsed  << "\n";
    std::cerr << "First 10 Items in first Row of data <internal_rep>(<decoded_rep>): \n";
    std::string osep = "";
    for (size_t i=0; i<std::min(data[0].size(),size_t{10}); i++){
        std::cerr << osep << data[0][i] << "("<< encoder.decode(data[0][i]) << ")";
        osep =  ", ";
    }
    std::cerr << "\n";

    if (do_nothing){
        std::cerr << "!!! Skipping algorithim execution !!! \n";
        return 0; 
    } 
    std::cerr << " ---- Algorithm ---- \n"; 
    auto hash_policy = ksets::hashing::HashPolicy<ksets::hashing::MurmurHash>(ksets::hashing::MurmurHash(seed));
    ksets::histogram::MapHistogramCollection<decltype(hash_policy)> hist_collection(number_of_clusters, hash_policy);
    std::cerr << "Preselect Strategy: " << preselect << "\n";
    std::cerr << "Histogram Construction: " << hist_rebuild << "\n";
    ksets::ClusterResult r;
    start = clock::now();
    if (hist_rebuild == "memorize") {
        if (preselect == "none") {
            r = r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterMemorizeChanges,
                ksets::HistPreselectNothing
                >(data, number_of_clusters, max_hist_length, hist_collection, max_iter, seed, cerr_sink); 
        } else if (preselect == "ibf") {
            r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterMemorizeChanges,
                ksets::HistPreselectIBF
                >(data, number_of_clusters, max_hist_length, hist_collection, max_iter, seed, cerr_sink);
        }
    } else if (hist_rebuild == "full"){
        if (preselect == "none") {
            r = r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterKsetsAuthors,
                ksets::HistPreselectNothing
                >(data, number_of_clusters, max_hist_length, hist_collection, max_iter, seed, cerr_sink); 
        } else if (preselect == "ibf") {
            r = ksets::cluster_cpp<
                decltype(hist_collection), 
                ksets::HistUpdaterKsetsAuthors,
                ksets::HistPreselectIBF
                >(data, number_of_clusters, max_hist_length, hist_collection, max_iter, seed, cerr_sink);
        }
    } else {
        throw std::runtime_error("Unknown hist_rebuild option: " + hist_rebuild +
                             ". Expected: 'memorize' or 'full'");
    }
    stop = clock::now();
    elapsed = std::chrono::duration<double>(stop - start).count();
    std::cerr << "Algorithm Total Runtime (seconds): " << elapsed << "\n"; 

    return 0;
}