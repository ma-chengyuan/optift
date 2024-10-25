#ifndef OPTIFT_COST_MODEL_H
#define OPTIFT_COST_MODEL_H

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace optift {

class FontCostModel { // NOLINT(*-special-member-functions)
  public:
    virtual double operator()(size_t n_glyphs) const = 0;
    virtual ~FontCostModel() = default;
};

class FontLinearCostModel : public FontCostModel {
  public:
    double cost_per_glyph;
    double cost_base;

    explicit FontLinearCostModel(
        std::span<const std::pair<size_t, double>> raw_data);

    double operator()(size_t n_glyphs) const override {
        return cost_per_glyph * static_cast<double>(n_glyphs) + cost_base;
    }
};

class FontEmpiricalCostModel : public FontCostModel {
  public:
    explicit FontEmpiricalCostModel(
        std::span<const std::pair<size_t, double>> raw_data);

    double operator()(size_t n_glyphs) const override;

  private:
    std::vector<std::pair<size_t, double>> data;
};

} // namespace optift

#endif