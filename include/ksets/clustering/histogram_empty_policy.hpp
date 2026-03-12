#pragma once

#include <vector>
#include <ksets/types.hpp>

namespace ksets {
    template <typename Derived>
    class EmptyClusterPolicy {
        protected:
            size_t K_;
            size_t iteration_ = 0;
        public:
            EmptyClusterPolicy(
                std::vector<cluster_t> &labels,
                size_t K)
                    : labels_(labels), K_(K) {}

            void begin_iteration(size_t iter){
                iteration_ = iter;
                static_cast<Derived*>(this)->begin_iteration_impl(iter);
            }

            void register_distances(size_t row, auto const& dists){
                static_cast<Derived*>(this)->register_distances_impl(row, dists);
            }

            void register_move(size_t row, cluster_t old_cluster, cluster_t new_cluster, dist_base_t dist){
                static_cast<Derived*>(this)->register_move_impl(row, old_cluster, new_cluster, dist);
            }

            void execute_fix(std::vector<cluster_t>& labels_cur){
                static_cast<Derived*>(this)->execute_fix_impl(labels_cur);
            }

        private:
            std::vector<cluster_t>& labels_; 
    };
}