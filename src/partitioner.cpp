#include "partitioner.h"

#include <bit>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/transform.hpp>

using namespace optift;

double PartitionInstance::eval(const PartitionSoln &soln) const {
    const std::vector<std::unordered_set<size_t>> &partitions = soln.partitions;

    if (partitions.size() != n_partitions) {
        throw std::runtime_error(
            fmt::format("invalid number of partitions: expected {}, got {}",
                        n_partitions, partitions.size()));
    }

    for (const auto &partition : partitions) {
        for (size_t i : partition) {
            if (i >= n_items) {
                throw std::runtime_error("invalid item index");
            }
        }
    }

    std::unordered_map<size_t, size_t> item_to_partition;
    for (size_t i = 0; i < partitions.size(); i++) {
        for (size_t item : partitions[i]) {
            item_to_partition[item] = i;
        }
    }
    if (item_to_partition.size() != n_items) {
        throw std::runtime_error(
            fmt::format("invalid number of covered items: expected {}, got {}",
                        n_items, item_to_partition.size()));
    }

    // std::vector<double> partition_costs;
    // partition_costs.reserve(partitions.size());
    // for (const auto &partition : partitions) {
    //     const auto size = static_cast<double>(partition.size());
    //     partition_costs.push_back(cost_model(size));
    // }

    using namespace ranges;

    const auto map = [](auto &mapping) {
        return views::transform([&mapping](auto i) { return mapping[i]; });
    };

    const std::vector<double> partition_costs =
        partitions |
        views::transform([&](const auto &p) { return cost_model(p.size()); }) |
        to<std::vector>;

    return accumulate(
        requests | views::transform([&](const auto &req) {
            const auto &[weight, items] = req;
            const std::unordered_set<size_t> partitions =
                items | map(item_to_partition) | to<std::unordered_set>;
            return weight * accumulate(partitions | map(partition_costs), 0.0);
        }),
        0.0);
}

PartitionSoln
optift::partition_solve_baseline(const PartitionInstance &instance) {
    std::vector<std::unordered_set<size_t>> partitions(instance.n_partitions);
    for (size_t i = 0; i < instance.n_items; i++)
        partitions[0].insert(i);
    return {partitions};
}

struct DynamicBitSet {
    using Element = uint64_t;
    constexpr static size_t ElementBits = sizeof(Element) * 8;

    size_t n_items;
    std::vector<Element> bits;

    explicit DynamicBitSet(size_t n)
        : bits((n + ElementBits - 1) / ElementBits, 0), n_items{n} {}

    template <std::ranges::input_range Container>
    explicit DynamicBitSet(size_t n, const Container &c) : DynamicBitSet{n} {
        for (size_t i : c) {
            set(i);
        }
    }

    size_t size() const {
        size_t count = 0;
        for (uint64_t word : bits) {
            count += std::popcount(word);
        }
        return count;
    }

    void set(size_t i) { bits[i / ElementBits] |= 1ULL << (i % ElementBits); }

    /// Returns a pair of bitsets, the first is the difference, the second is
    /// the intersection with the other bitset.
    std::pair<DynamicBitSet, DynamicBitSet>
    diff_intersect_with(const DynamicBitSet &other) const {
        // Can make do if sizes are different, but not doing that for now.
        assert_same_size(other);

        DynamicBitSet diff{n_items};
        DynamicBitSet inter{n_items};
        for (size_t i = 0; i < bits.size(); i++) {
            diff.bits[i] = bits[i] & ~other.bits[i];
            inter.bits[i] = bits[i] & other.bits[i];
        }
        return {diff, inter};
    }

    DynamicBitSet union_with(const DynamicBitSet &other) const {
        assert_same_size(other);
        DynamicBitSet result{n_items};
        for (size_t i = 0; i < bits.size(); i++) {
            result.bits[i] = bits[i] | other.bits[i];
        }
        return result;
    }

    bool is_disjoint(const DynamicBitSet &other) const {
        assert_same_size(other);
        for (size_t i = 0; i < bits.size(); i++) {
            if (bits[i] & other.bits[i]) {
                return false;
            }
        }
        return true;
    }

    std::unordered_set<size_t> to_set() const {
        std::unordered_set<size_t> result;
        for (size_t i = 0; i < bits.size(); i++) {
            for (Element t = bits[i]; t; t &= t - 1) {
                const size_t tz = std::countr_zero(t);
                result.insert(i * ElementBits + tz);
            }
        }
        return result;
    }

  private:
    void assert_same_size(const DynamicBitSet &other) const {
        if (n_items != other.n_items) {
            throw std::runtime_error{
                fmt::format("bitsets have different sizes: {} and {}", n_items,
                            other.n_items)};
        }
    }
};

struct HeuristicPartition {
    std::unordered_set<size_t> reqs;
    DynamicBitSet items;
};

PartitionSoln
optift::partition_solve_heuristic(const PartitionInstance &instance,
                                  PartitionSoln initial_soln) {

    const std::vector<std::pair<double, DynamicBitSet>> r =
        instance.requests | ranges::views::transform([&](const auto &req) {
            const auto &[w, items] = req;
            DynamicBitSet set{instance.n_items, items};
            return std::pair{w, set};
        }) |
        ranges::to<std::vector>();

    std::unordered_map<size_t, std::unordered_set<size_t>> item_to_reqs;
    for (const auto &[i, w_items] :
         instance.requests | ranges::views::enumerate) {
        const auto &[_, items] = w_items;
        for (size_t item : items) {
            if (const auto it = item_to_reqs.find(item);
                it != item_to_reqs.end()) {
                it->second.insert(i);
            } else {
                item_to_reqs[item] = {i};
            }
        }
    }

    std::vector<HeuristicPartition> p =
        initial_soln.partitions |
        ranges::views::transform([&](const auto &partition) {
            HeuristicPartition part{
                .reqs = {},
                .items = DynamicBitSet{instance.n_items},
            };
            for (const auto item : partition) {
                const auto &reqs = item_to_reqs.at(item);
                part.reqs.insert(reqs.begin(), reqs.end());
                part.items.set(item);
            }
            return part;
        }) |
        ranges::to<std::vector>();

    double cur_cost = instance.eval(initial_soln);
    const auto &cost = instance.cost_model;
    bool can_improve = true;
    for (int iter = 0; can_improve; iter++) {
        can_improve = false;
        for (size_t c = 0; c < r.size(); c++) {
            double best_cost = cur_cost;
            std::optional<std::tuple<size_t, HeuristicPartition, size_t,
                                     HeuristicPartition>>
                best_move;
            const auto &[_, items] = r[c];
            for (size_t i = 0; i < p.size(); i++) {
                const HeuristicPartition &p1 = p[i];
                const auto [items_retained, items_removed] =
                    p1.items.diff_intersect_with(items);
                std::unordered_set<size_t> reqs_retained;
                std::unordered_set<size_t> reqs_affected;
                double reqs_retained_weight = 0.0;
                double reqs_removed_weight = 0.0;
                for (const auto u : p1.reqs) {
                    const auto &[weight, items_u] = r[u];
                    if (items_u.is_disjoint(items_retained)) {
                        reqs_removed_weight += weight;
                    } else {
                        reqs_retained_weight += weight;
                        reqs_retained.insert(u);
                    }
                    if (!items_u.is_disjoint(items_removed)) {
                        reqs_affected.insert(u);
                    }
                }

                const size_t size_before = p1.items.size();
                const size_t size_after = items_retained.size();
                const double cost_after_ban =
                    cur_cost - (reqs_removed_weight * cost(size_before)) -
                    (reqs_retained_weight *
                     (cost(size_before) - cost(size_after)));

                // Try to move items_removed to another partition j
                for (size_t j = 0; j < p.size(); j++) {
                    if (i == j) {
                        // Cannot move to the same partition
                        continue;
                    }
                    const HeuristicPartition &p2 = p[j];
                    const DynamicBitSet items_extended =
                        p2.items.union_with(items_removed);
                    const size_t size_before = p2.items.size();
                    const size_t size_after = items_extended.size();
                    double reqs_existing_weight = 0.0;
                    for (const auto u : p2.reqs) {
                        const auto &[weight, _] = r[u];
                        reqs_existing_weight += weight;
                    }
                    double reqs_extended_weight = 0.0;
                    for (const auto u : reqs_affected) {
                        if (!p2.reqs.contains(u)) {
                            const auto &[weight, _] = r[u];
                            reqs_extended_weight += weight;
                        }
                    }
                    const double cost_after_add =
                        cost_after_ban +
                        ((cost(size_after) - cost(size_before)) *
                         reqs_existing_weight) +
                        (cost(size_after) * reqs_extended_weight);
                    if (cost_after_add < best_cost) {
                        best_cost = cost_after_add;
                        std::unordered_set<size_t> new_p2_reqs = p2.reqs;
                        for (const auto u : reqs_affected) {
                            new_p2_reqs.insert(u);
                        }
                        const HeuristicPartition new_p1{
                            .reqs = reqs_retained,
                            .items = items_retained,
                        };
                        const HeuristicPartition new_p2{
                            .reqs = std::move(new_p2_reqs),
                            .items = items_extended,
                        };
                        best_move = {i, new_p1, j, new_p2};
                    }
                }
            }
            if (best_move.has_value()) {
                auto [i, new_p1, j, new_p2] = std::move(best_move.value());
                can_improve = true;
                p[i] = std::move(new_p1);
                p[j] = std::move(new_p2);
                spdlog::debug(
                    "iter {:03} cost: {:11.6f} -> {:11.6f} (ban {:02} "
                    "from {:02} and "
                    "join {:02})",
                    iter, cur_cost, best_cost, c, i, j);
                cur_cost = best_cost;
            }
        }
    }
    PartitionSoln soln{
        p | ranges::views::transform([](const auto &part) {
            return part.items.to_set();
        }) | ranges::to<std::vector>(),
    };
    ranges::sort(soln.partitions, std::less<>{},
                 [](const auto &a) { return -static_cast<int>(a.size()); });
    return soln;
}