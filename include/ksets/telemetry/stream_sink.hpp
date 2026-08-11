#pragma once

#include <ksets/telemetry/base.hpp>
#include <ksets/telemetry/events.hpp>

namespace ksets::telemetry {


template<class Stream>
struct StreamSink {
    // Maximum telemetry level this sink will handle
    static constexpr ksets::telemetry::TelemetryLevel level =
        ksets::telemetry::TelemetryLevel::basic;

    explicit StreamSink(Stream& s)
        : out_(s)
    {}

    Stream& out_;

    template<class Event>
    void handle(const Event& e) const {
        print(out_, e);
    }

private:
    mutable size_t savings_events = 0;
    mutable double savings_preselect = 0.0;

    // Generic fallback (if no specific overload exists)
    template<class Event>
    void print(std::ostream& out, const Event& e) const {
        out << "[Telemetry] Event: " << typeid(e).name() << "\n";
    }

    void print(std::ostream& out, const ksets::PreselectSavingsTelemetry& e) const {
        savings_preselect += 1.0 - double(e.total_hists_touched) / double(e.total_tokens * e.K);
        savings_events++;
          
    }

    void print(std::ostream& out, const ksets::AlgorithmStart& e ) const {
        out << "[Algorithm Initialized]\n" 
                  << "Histograms Provided: " << (e.histograms_provided ? "TRUE" : "FALSE") 
                  << "\nData Size: " << e.n_items << "\n";
    }

    void print(std::ostream& out, const ksets::DistanceMeasureTelemetry& e) const {
        out << "[Iteration " << e.iter << ", Data Row: " << e.data_row << " ]" ;
        size_t iter = 0;
        std::string sep = " "; 
        for ( const auto &dist: e.distances) {
            out << sep << iter << ": " << dist;
            ++iter;
            sep = ", ";
        }
        out <<  "\n";
    }

    void print(std::ostream& out, const ksets::HistogramUpdateInfo& e) const {
        out << "[Iteration " << e.iteration << "] Histogram Updates = " << e.changes << "\n" ;
    }

    void print(std::ostream& out, const ksets::LabelSelectTelemetry& e) const {
        out << "[Iteration " << e.iter << ", Data Row: " << e.data_row << " ] " 
                 << "Dist( " << e.old_label << " ) = " << e.old_dist << " --> "
                 << "Dist( " << e.new_label << " ) = " << e.new_dist << "\n";
    }

    // IterationMetrics specialization
    void print(std::ostream& out, const ksets::IterationMetrics& e) const {
        
        out << "[Iteration " << e.iter << "] "
                  << "Objective: " << e.objective
                  << ", Delta: " << e.delta;
        if (savings_events > 0){
            double avg_savings = savings_preselect / static_cast<double>(savings_events);
            out << ", Avg Preselect Savings: " << avg_savings * 100 << "%";  
        }          
        savings_events = 0;
        savings_preselect = 0.0;
        out << "\n"; 
    }

    // RunSummary example
    void print(std::ostream& out, const ksets::RunSummary& e) const {
        std::string str_reason; 
        switch (e.reason){
            case ksets::EndReason::DistanceSumStagnated:
                str_reason = "Sum of Distances Stagnated";
                break;
            case ksets::EndReason::DistanceIncreased:
                str_reason = "Sum of Distances Increased after Assignment";
                break;
            case ksets::EndReason::IterationLimitReached:
                str_reason = "Max Limit of Iteration Reached";
                break;
            case ksets::EndReason::Interupt:
                str_reason = "Algorithm was Interupted by System or User";
                break;
        }
        size_t num_empty = 0; 
        for (auto len: e.hist.lengths){
            if (len != 0){
                break;
            }
            num_empty++;
        }
        size_t k = e.hist.lengths.size();
        size_t num_not_empty = k - num_empty;
        out << "[ Final Summary ]\n";     
        out << "Total Iterations: " << e.iterations << "\n"
                << "End Reason: " << str_reason << "\n"
                << "--- Histogram Stats --- \n"
                << "Non-Empty Histograms: " << num_not_empty << "\n"
                << "Min Histogram Size: " << e.hist.min_len << ", Max Histogram Size: " << e.hist.max_len << "\n" 
                << "Avg Histogram Size: " << e.hist.avg_len << "\n"
                << "Median Histogram Size: " << e.hist.median_len << "\n"
                << "Smallest non-empty Histogram has Label: " << e.hist.label_sortorder[num_empty] << "\n"
                << "  Hist Size: " <<  e.hist.lengths[num_empty] 
                    << ", Minimum Value: " << e.hist.min_vals[num_empty] 
                    << " , Maximum Value: " << e.hist.max_vals[num_empty] << "\n"
                << "Largest Histogram has Label:            " << e.hist.label_sortorder[k-1] << "\n"
                << "  Hist Size: " <<  e.hist.lengths[k-1] 
                    << ", Minimum Value: " << e.hist.min_vals[k-1] 
                    << " , Maximum Value: " << e.hist.max_vals[k-1] << "\n"
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
            out << "[Run Summary] "
                    << "Iterations: " << e.iterations
                    << ", AvgHist: " << e.avg_hist
                    << ", MedianHist: " << e.median_hist
                    << ", SumDistances: " << e.sum_distances
                    << "\n";*/
    }
};




}