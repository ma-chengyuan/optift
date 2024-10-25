#include "input.h"

#include <set>

#include <unicode/schriter.h>

namespace optift {

std::unordered_set<std::string> Input::get_unique_font_paths() const {
    std::unordered_set<std::string> result;
    for (const auto &[_, font] : fonts) {
        result.insert(font.path);
    }
    return result;
}

std::vector<UChar32>
Input::get_all_codepoints_sorted(const std::string &font_path) const {
    std::set<UChar32> codepoints_set;
    for (const auto &style : get_styles_with_font_path(font_path)) {
        for (const auto &[_, post] : posts) {
            if (const auto it_str = post.codepoints.find(style);
                it_str != post.codepoints.end()) {
                icu::StringCharacterIterator it_ch{it_str->second};
                for (it_ch.setToStart(); it_ch.hasNext();) {
                    const auto c = it_ch.next32PostInc();
                    codepoints_set.insert(c);
                }
            }
        }
    }
    return {codepoints_set.begin(), codepoints_set.end()};
}

} // namespace optift