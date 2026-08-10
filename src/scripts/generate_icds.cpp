#include <scripts/generate_icds.hpp>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <sharg/parser.hpp>
#include <sharg/validators.hpp>



// Gaussian tail approximation (1 - CDF)
inline double normal_tail(double z) {
    // Abramowitz-Stegun approximation
    return 0.5 * std::erfc(z / std::sqrt(2.0));
}

inline double keep_probability(size_t thresh, double mu, double sigma){
    double z = (thresh - mu) / sigma;
    return normal_tail(z);
} 


int main(int argc, char ** argv) {

    uint number_of_clusters = 1000;
    uint number_of_samples = 10000;
    bool do_nothing = false;
    double density = 0.01;
    double inner_jaccard = 0.8;
    size_t sample_length = 1000;
    size_t seed = 12342321;
    std::string prefix{}; 
    try
    {
        // ---------------------------
        // Define CLI parser
        // ---------------------------
        sharg::parser parser{"genicds", argc, argv, sharg::update_notifications::off};
        
        parser.info.short_description = "Generate Test Data for Clustering Eval Inspired by ICD Codes";
        parser.info.version = "0.1.0";
        parser.add_option(prefix, sharg::config{.short_id = 'p', .long_id = "prefix", 
            .description = "Output Prefix: Output files share this prefix.\n" 
                           "You should see a <prefix>.rep.txt and <prefix>.txt\n"  
                           "after running this tool with a prefix specified.\n" 
                           "Otherwise the prefix will be 'sim_icd_<num_samples>_<num_clusters>_<length_per_sample>'" });
        parser.add_option(number_of_clusters, sharg::config{
                .short_id = 'k', .long_id = "clusters", 
                .description = "Number of Clusters"});
        //parser.add_option(max_hist_length, sharg::config{.short_id = 'm', .long_id = "max-hist", .description = "Maximum Histogram length"});
        parser.add_option(number_of_samples, sharg::config{
                .short_id = 'n', .long_id = "sample_num", 
                .description = "Number of samples to generate"});
        parser.add_option(inner_jaccard, sharg::config{
                .short_id = 'j', .long_id = "jaccard",
                .description="Expected Jaccard-Similarty between two members of the same cluster. Default: 0.80"});
        parser.add_option(density, sharg::config{
                .short_id = 'd', .long_id = "density",
                .description="Density of the Universe from which the cluster values are sampled. Default: 0.01\n" 
                             "This together with the number of samples, determines the random overlap between clusters"});
        parser.add_option(sample_length, sharg::config{
                .short_id = 'l', .long_id = "sample_length",
                .description="Length of generated samples. Defaults to 1000"});
        parser.add_option(seed, sharg::config{
                .short_id = 's', .long_id = "seed", 
                .description = "Seed (optional, default=12342321)"});
        parser.add_flag(do_nothing, sharg::config{.short_id = 'D', .long_id = "do_nothing", .description = "Only runs file IO"});
        parser.parse();

        if (prefix.empty()){
            std::stringstream prefix_buffer;
            prefix_buffer << "sim_icd";
            prefix_buffer << "_" << number_of_samples << "_" << number_of_clusters << "_" << sample_length;
            prefix = prefix_buffer.str();

        }
        double ibf_fpr = 0.005 / 64.0;
        size_t universe_size = static_cast<size_t>(sample_length * (inner_jaccard + 1.0) / (2*inner_jaccard * density));
        size_t absolute_overlap = static_cast<size_t>(std::ceil(sample_length * density * ( inner_jaccard + 1 ) / (2.0 * inner_jaccard)));
        // Sigmas and Means 
        double p_overlap = absolute_overlap / double(sample_length);
        double var_overlap = absolute_overlap * p_overlap * (1.0 - p_overlap);
        double var_ibf = ibf_fpr * sample_length * (1.0 - ibf_fpr);
        size_t mu_ibf = ibf_fpr * sample_length;
        double sigma_fp = std::sqrt(var_overlap + var_ibf);
        size_t mu_fp = absolute_overlap + mu_ibf;
        double p_tp = (2.0 * inner_jaccard) / (1.0 + inner_jaccard);
        double mu_tp = sample_length * p_tp;
        double sigma_tp = std::sqrt(sample_length * p_tp * (1.0 - p_tp));
        double sigma_total = std::sqrt((var_overlap + var_ibf) + (sigma_tp * sigma_tp));

        double separation = (mu_tp - mu_fp) / sigma_total;
        //double separation = (mu_tp - mu_fp) / sigma_fp;
        double optimal_threshold =  static_cast<size_t>(std::ceil(mu_fp + 3 * sigma_fp));
        double sqrt_threshold = static_cast<size_t>(.9999 + std::sqrt(sample_length) * 0.1);
        double optimal_keep_fp = keep_probability(optimal_threshold, mu_fp, sigma_fp);
        double optimal_keep_tp = keep_probability(optimal_threshold, mu_tp, sigma_tp);
        double optimal_expected_candidates = optimal_keep_tp + (number_of_clusters - 1) * optimal_keep_fp;  // This breaks for small numbers of clusters 
        double optimal_efficiency = 1.0 - optimal_expected_candidates / double(number_of_clusters);         // and high density values. 
        double sqrt_keep_fp = keep_probability(sqrt_threshold, mu_fp, sigma_fp);                            //  
        double sqrt_keep_tp = keep_probability(sqrt_threshold, mu_tp, sigma_tp);                            //
        double sqrt_expected_candidates = sqrt_keep_tp + (number_of_clusters - 1) * sqrt_keep_fp;           //  
        double sqrt_efficiency = 1.0 - sqrt_expected_candidates / double(number_of_clusters);               //
        // ---------------------------
        // Print values for testing
        // ---------------------------
        std::cerr << "Output Prefix: " << prefix << "\n"
         << "Seed: " << seed << "\n"
         << "Number of Clusters: " << number_of_clusters << "\n"
         << "Number of Samples: " << number_of_samples << "\n"
         << "Sample Length: " << sample_length << "\n"
         << "Pairwise Jaccard Similarity within Clusters: " << inner_jaccard << "\n"
         << "Density: " << density << "\n\n"
         << "-- Derived Values -- " << "\n"
         << "Universe Size (from which cluster values are sampled): " 
            << universe_size << "\n"
         << "Expected Overlap (absolute) between unrelated clusters: " 
            << absolute_overlap << "\n"
         << "Expected Overlap (relative) between unrelated clusters: " 
            << absolute_overlap / static_cast<double>( sample_length) << "\n"
         << "\n" 
         << "-- IBF info --\n"
         << "IBF Expected False Positives (FPR "<< ibf_fpr <<"): " << mu_ibf << "\n" 
         << "IBF Expected Total Overlap Between Unrelated: " << mu_ibf + absolute_overlap << "\n"
         << "IBF Aprox. Expected Overlap Between Related: " << mu_tp << "\n"
         << "IBF Simple SQRT Threshold: " <<  sqrt_threshold << "\n" 
         << "IBF Optimal     Threshold: " << optimal_threshold << "\n"
         << "IBF Cluster Separability (Z-score): " << separation << "\n"
         << "IBF Efficency with Optimal Threshold:     " << optimal_efficiency << "\n"  
         << "IBF Efficency with Simple Sqrt Threshold: " << sqrt_efficiency << "\n" 
            << "\n"; 

    }
    catch (sharg::parser_error const & ext) // catch user errors
    {
        std::cerr << "Parsing error: " << ext.what() << "\n";
        return -1;
    }
    std::mt19937_64 rng(seed);
     
    std::vector<genicds::Group> groups;
    std::vector<std::vector<genicds::pcode_t>> rep_members(number_of_clusters);
    std::vector<size_t> number_members(number_of_clusters,0);
    std::vector<size_t> inner_jaccard_comps(0,number_of_clusters);
    std::vector<size_t> outer_jaccard_comps(0,number_of_clusters);
    std::vector<std::vector<genicds::pcode_t>> rolling_rep(number_of_clusters);

    std::vector<std::unordered_map<genicds::pcode_t,size_t>> histograms(number_of_clusters); 
    for(genicds::gcode_t i=0;i<number_of_clusters;i++) {
        groups.push_back({
            genicds::gcode_t(i),
            double(inner_jaccard),
            double(density),
            size_t(sample_length)
        });
    }
    
    if (do_nothing){
        std::cerr << "Specified a dryrun and will skip data generation.\n Remove the -D or --do_nothing flag to run properly\n";
        return 0;
    }
    std::ofstream out(prefix  + ".txt");
    std::ofstream rep_out(prefix + ".rep.txt");
    out.rdbuf()->pubsetbuf(nullptr, 1<<20); // 1MB buffer
    std::vector<genicds::pcode_t> pcode_buffer;
    pcode_buffer.reserve(static_cast<size_t>(sample_length * 1.2));
    for (uint64_t pid = 0; pid < number_of_samples; ++pid) {

        auto sample = genicds::generate_patient(groups,rng);
        if (rep_members[sample.primary_g_id].empty())
            rep_members[sample.primary_g_id] = sample.codes;
        //out << '';//pid;
        number_members[sample.primary_g_id]++;
        bool first = true; 
        size_t i=0;
        for (auto code : sample.codes){
            if (!first){
                out << ' ';
            }
            auto pcode = genicds::extract_local(code);
            auto gcode = genicds::extract_group(code);
            out << pcode;
            first = false;
            auto [it, inserted] = histograms[gcode].try_emplace(pcode, 1);
            if (!inserted)
                ++it->second;
            i++;
        }
        out << '\n';
    }
    std::cerr << "Wrote Simulated Data to: " << prefix  << ".txt" << "\n";
    //Output representatives 
    for (auto &rep : rep_members ){
        bool first = true;
        for (auto code : rep) {
            if (!first)
                rep_out << ' ';
            rep_out << genicds::extract_local(code);
            first = false;
        }
        rep_out << "\n";
    }
    //Approx Overlaps Between Clusters
    size_t approx_overlaps = 0;
    size_t comparisons = 0;

    for (size_t i=1; i<number_of_clusters; ++i) {
        for (size_t j = 0; j < i; ++j) {
            auto & prev_hist = histograms[j];
            for (auto &[pcode, count]: histograms[i]) {
                auto it = prev_hist.find(pcode);
                approx_overlaps += (it != prev_hist.end());
            }
            comparisons++;
        }
    }
    
    approx_overlaps /= comparisons; 
    std::cerr << "Observed average overlap (absolute) between clusters: " << approx_overlaps << "\n"; 
    std::vector<size_t> histogram_sums(number_of_clusters,0);
    std::vector<size_t> histogram_maxs(number_of_clusters,0);
    std::vector<size_t> histogram_mins(number_of_clusters, std::numeric_limits<size_t>::max());
    for (size_t i=0; i < number_of_clusters; ++i ) {
        for (auto &[pcode, count]: histograms[i]) {
            histogram_sums[i] += count;
            if (histogram_maxs[i] < count) 
                histogram_maxs[i] = count;
            if (histogram_mins[i] > count) 
                histogram_mins[i] = count;
        }
    }
    size_t max_sum_cluster = std::distance(histogram_sums.begin(), std::max_element(histogram_sums.begin(), histogram_sums.end()));
    size_t min_sum_cluster = std::distance(histogram_sums.begin(), std::min_element(histogram_sums.begin(), histogram_sums.end()));
    std::cout << "Largest Cluster by Sum: [" << max_sum_cluster << "] -> Sum: " 
              << histogram_sums[max_sum_cluster] 
              << " Max(val): " <<  histogram_maxs[max_sum_cluster]
              << " Min(val): " <<  histogram_mins[max_sum_cluster]
              << "\n" ;
    std::cout << "Smallest Cluster by Sum: [" << min_sum_cluster << "] -> Sum: " 
              << histogram_sums[min_sum_cluster] 
              << " Max(val): " <<  histogram_maxs[min_sum_cluster]
              << " Min(val): " <<  histogram_mins[min_sum_cluster]
              << "\n" ; 
}