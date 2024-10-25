#ifndef OPTIFT_PARTITIONER_H
#define OPTIFT_PARTITIONER_H

#include <cstddef>
#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace optift {

using CostModel = std::function<double(size_t)>;

struct PartitionSoln {
    std::vector<std::unordered_set<size_t>> partitions;
};

struct PartitionInstance {
    // The number of partitions in the instance
    size_t n_partitions;
    // The number of items in the instance
    size_t n_items;
    // Request as (weight, set of item) pairs
    std::vector<std::pair<double, std::unordered_set<size_t>>> requests;

    CostModel cost_model;

    double eval(const PartitionSoln &soln) const;
};

PartitionSoln partition_solve_baseline(const PartitionInstance &instance);

PartitionSoln partition_solve_heuristic(const PartitionInstance &instance,
                                        PartitionSoln initial_soln);

} // namespace optift

#endif