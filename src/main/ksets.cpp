#include <sharg/parser.hpp>
#include <iostream>
#include <string>

// Stub was generated using ChatGTP and the following prompt
/*
 I want to have an executable stub ksets.cpp this is the inspiration:
  def cluster(data, number_of_cluster, max_hist_length, histograms = [], max_iter = 0): if max_iter == 0: max_iter = 50 .... 
  i want a small sharg parser that accepts a file as data and int number of clusters and optional max_hist_length and max_iter
*/ 

int main(int argc, char ** argv)
{
    try
    {
        // ---------------------------
        // Define CLI parser
        // ---------------------------
        sharg::parser parser{"ksets", argc, argv, sharg::update_notifications::off};

        std::string data_file;
        int number_of_clusters{};
        int max_hist_length{100};  // default value
        int max_iter{50};          // default value

        parser.add_option(data_file, sharg::config{.short_id = 'f', .long_id = "file", .description = "Input File"});
        parser.add_option(number_of_clusters, sharg::config{.short_id = 'k', .long_id = "clusters", .description = "Number of Clusters"});
        //parser.add_option(max_hist_length, sharg::config{.short_id = 'm', .long_id = "max-hist", .description = "Maximum Histogram length"});
        //parser.add_option(max_iter, 'i', "max-iter", "Maximum number of iterations (optional)", sharg::option_spec::DEFAULT);

        parser.parse();

        // ---------------------------
        // Print values for testing
        // ---------------------------
        std::cout << "Data file: " << data_file << "\n";
        std::cout << "Number of clusters: " << number_of_clusters << "\n";
        std::cout << "Max histogram length: " << max_hist_length << "\n";
        std::cout << "Max iterations: " << max_iter << "\n";

        // ---------------------------
        // TODO: call your clustering function here
        // cluster(data_file, number_of_clusters, max_hist_length, max_iter);
        // ---------------------------

    }
    catch (sharg::parser_error const & ext) // catch user errors
    {
        std::cerr << "Parsing error: " << ext.what() << "\n";
        return -1;
    }

    return 0;
}