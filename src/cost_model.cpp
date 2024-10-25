#include <cost_model.h>

#include <map>

// #include <range/v3/all.hpp>
#include <range/v3/algorithm/lower_bound.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

using namespace optift;

FontLinearCostModel::FontLinearCostModel(
    std::span<const std::pair<size_t, double>> raw_data) {
    const double n = static_cast<double>(raw_data.size());
    const auto [s_x, s_y, s_xx, s_xy] = ranges::accumulate(
        raw_data, std::tuple{0.0, 0.0, 0.0, 0.0},
        [](const auto &acc, const auto &pair) {
            const auto [s_x, s_y, s_xx, s_xy] = acc;
            const auto [x, y] = pair;
            return std::tuple{s_x + x, s_y + y, s_xx + x * x, s_xy + x * y};
        });
    cost_per_glyph = (n * s_xy - s_x * s_y) / (n * s_xx - s_x * s_x);
    cost_base = (s_y - cost_per_glyph * s_x) / n;
}

FontEmpiricalCostModel::FontEmpiricalCostModel(
    std::span<const std::pair<size_t, double>> raw_data) {
    std::map<size_t, std::pair<double, size_t>> data_map;
    for (const auto &[n_glyphs, cost] : raw_data) {
        if (const auto it = data_map.find(n_glyphs); it != data_map.end()) {
            auto &[sum, count] = it->second;
            sum += cost;
            count++;
        } else {
            data_map[n_glyphs] = {cost, 1};
        }
    }
    data = data_map | ranges::views::transform([](const auto &pair) {
               const auto [sum, count] = pair.second;
               return std::pair{pair.first, sum / count};
           }) |
           ranges::to<std::vector>;
}

double FontEmpiricalCostModel ::operator()(size_t n_glyphs) const {
    const auto get_x = [](const auto &pair) { return pair.first; };
    const auto ub = ranges::lower_bound(data, n_glyphs, {}, get_x);
    if (ub == data.end()) {
        return data.back().second;
    }
    if (ub == data.begin()) {
        return data.front().second;
    }
    if (ub->first == n_glyphs) {
        return ub->second;
    }
    const auto lb = ub - 1;
    return lb->second + (ub->second - lb->second) *
                            static_cast<double>(n_glyphs - lb->first) /
                            static_cast<double>(ub->first - lb->first);
}
