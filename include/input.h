#ifndef OPTIFT_INPUT_H
#define OPTIFT_INPUT_H

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/adl_serializer.hpp>
#include <nlohmann/json.hpp>

#include <range/v3/view/filter.hpp>
#include <range/v3/view/map.hpp>

#include <unicode/unistr.h>

using json = nlohmann::json;

namespace nlohmann {

template <> struct adl_serializer<icu::UnicodeString> {
    static void to_json(json &j, const icu::UnicodeString &s) {
        std::string temp;
        s.toUTF8String(temp);
        j = temp;
    }

    static void from_json(const json &j, icu::UnicodeString &s) {
        std::string temp;
        j.get_to(temp);
        s = icu::UnicodeString::fromUTF8(temp);
    }
};

} // namespace nlohmann

namespace optift {

struct FontSpec {
    std::string path;
    std::map<std::string, std::string> css;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FontSpec, path, css);
};

struct InputPost {
    double weight;
    std::unordered_map<std::string, icu::UnicodeString> codepoints;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InputPost, weight, codepoints);
};

struct Input {
    std::unordered_map<std::string, FontSpec> fonts;
    std::unordered_map<std::string, InputPost> posts;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Input, fonts, posts);

    std::unordered_set<std::string> get_unique_font_paths() const;

    std::vector<UChar32>
    get_all_codepoints_sorted(const std::string &font_path) const;

    auto get_styles_with_font_path(const std::string &font_path) const {
        using namespace ranges::views;
        // clang-format off
        return fonts |
               filter([&](const auto &p) { return p.second.path == font_path; }) |
               keys;
        // clang-format on
    }
};

} // namespace optift

#endif