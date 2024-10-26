
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <random>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <argparse/argparse.hpp>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <hb-subset.h>
#include <hb.h>
#include <indicators/block_progress_bar.hpp>
#include <indicators/setting.hpp>
#include <spdlog/spdlog.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/task_group.h>
#include <unicode/schriter.h>
#include <unicode/umachine.h>
#include <unicode/unistr.h>
#include <woff2/encode.h>
#include <zlib.h>

#include <range/v3/algorithm/is_sorted.hpp>
#include <range/v3/algorithm/sort.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/chunk.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/set_algorithm.hpp>
#include <range/v3/view/transform.hpp>

#include "cost_model.h"
#include "hb_wrap.h"
#include "input.h"
#include "partitioner.h"

using namespace optift;

constexpr int RNG_SEED = 42;
constexpr int NUM_SAMPLES = 100;

/**
 * Builds a cost model for the given font face and codepoint universe, with
 * specified RNGs and number of samples. On a high-level, it samples subsets of
 * the universe and look at the size of the subsetted font files.
 *
 * This can be compute-intensive since many subsetting and compression are
 * performed, so it is parallelized with TBB and cached.
 *
 * \param face The font face to build the cost model for
 * \param codepoints A span of codepoints to build the cost model for
 * \param rng_seed The seed for the RNG
 * \param n_samples The number of samples to take
 * \return The built cost model
 */
CostModel build_cost_model(hb_face_t *face, std::span<const UChar32> codepoints,
                           unsigned long rng_seed, int n_samples);

/**
 * Creates an abstract partition instance from the input data for a given font
 * path and cost model.
 *
 * \param input The input data
 * \param font_path The font path to create the partition instance for
 * \param cost_model The cost model to use
 * \param n_partitions The number of partitions to create
 * \return A pair of the partition instance and a vector mapping item index to
 *   codepoint. This is used to re-map an abstract solution back to codepoint
 *   partitions.
 */
std::pair<PartitionInstance, std::vector<UChar32>>
create_partition_instance(const Input &input, const std::string &font_path,
                          CostModel cost_model, size_t n_partitions);

/**
 * Represents a partitioning of a single font that can be wrtten to disk and
 * served.
 */
struct FontPartitionSoln {
    std::string css; // The CSS for this partitioning
    // Vector of (filename, data) pairs for the subsetted fonts
    std::vector<std::pair<std::string, std::vector<uint8_t>>> subsetted_fonts;
    std::unordered_map<UChar32, size_t> codepoint_to_partition;

    static FontPartitionSoln
    from_partition_soln(const Input &input, const std::string &font_path,
                        hb_face_t *face, const PartitionInstance &instance,
                        const PartitionSoln &soln,
                        std::span<const UChar32> item_to_codepoint);

    static FontPartitionSoln from_google_fonts(const Input &input,
                                               const std::string &font_path,
                                               hb_face_t *face,
                                               bool subset = false);
};

/**
 * Saves the solution to a CSS file and evaluates the solution.
 *
 * \param input The input data
 * \param font_path The font path of the font
 * \param face The font face
 * \param instance The partition instance from \ref create_partition_instance
 * \param soln The partition solution
 * \param item_to_codepoint The mapping from item index to codepoint, also
 *   from \ref create_partition_instance
 */
void save_and_evaluate_solution(const Input &input,
                                const std::string &font_path, hb_face_t *face,
                                const PartitionInstance &instance,
                                const PartitionSoln &soln,
                                std::span<const UChar32> item_to_codepoint,
                                const argparse::ArgumentParser &program);

int main(int argc, char **argv) {
    argparse::ArgumentParser program{"optift"};
    program.add_argument("-i", "--input")
        .help("path to the input JSON file")
        .required();
    program.add_argument("-o", "--output")
        .help("path to the output directory")
        .required();
    program.add_argument("-n", "--n-partitions")
        .help("number of partitions to create")
        .required()
        .scan<'i', int>();
    program.add_argument("--rng")
        .help("RNG seed for sampling cost model")
        .default_value(RNG_SEED)
        .scan<'i', int>();
    program.add_argument("--samples")
        .help("number of samples for cost model")
        .default_value(NUM_SAMPLES)
        .scan<'i', int>();
    program.add_argument("--compare-baseline")
        .help("compare heuristic solution to baseline solution")
        .flag();
    program.add_argument("--compare-google")
        .help("compare heuristic solution to Google Fonts solution")
        .flag();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    const int rnd_seed = program.get<int>("--rng");
    const int n_samples = program.get<int>("--samples");
    const int n_partitions = program.get<int>("--n-partitions");

    const std::filesystem::path output_path{
        program.get<std::string>("--output")};
    std::filesystem::remove_all(output_path);
    if (!std::filesystem::exists(output_path)) {
        std::filesystem::create_directories(output_path);
    }

    std::ifstream f{program.get<std::string>("--input")};
    const json j = json::parse(f);
    const Input input = j.get<Input>();

    for (const auto &font_path : input.get_unique_font_paths()) {
        const std::vector<UChar32> codepoints =
            input.get_all_codepoints_sorted(font_path);

        spdlog::info("font path: {} ({} codepoints used)", font_path,
                     codepoints.size());

        const BlobPtr blob{hb_blob_create_from_file_or_fail(font_path.data())};
        const FacePtr face{hb_face_create(blob.get(), 0)};

        spdlog::info("fitting cost model...");
        const auto cost_model =
            build_cost_model(face.get(), codepoints, rnd_seed, n_samples);
        const auto [instance, item_to_codepoint] = create_partition_instance(
            input, font_path, cost_model, n_partitions);

        const PartitionSoln soln_baseline = partition_solve_baseline(instance);
        spdlog::info("baseline cost: {}", instance.eval(soln_baseline));
        const PartitionSoln soln_heuristic =
            partition_solve_heuristic(instance, soln_baseline);
        spdlog::info("heuristic cost: {}", instance.eval(soln_heuristic));

        save_and_evaluate_solution(input, font_path, face.get(), instance,
                                   soln_heuristic, item_to_codepoint, program);
    }
    return 0;
}

/**
 * Generates a CSS unicode-range string that covers all codepoints given.
 *
 * \param sorted_codepoints A sorted span of codepoints
 * \return A CSS unicode-range string
 */
std::string generate_unicode_range(std::span<const UChar32> sorted_codepoints) {
    if (!ranges::is_sorted(sorted_codepoints)) {
        throw std::invalid_argument("codepoints must be sorted");
    }
    std::string result;
    const auto append = [&result](const std::string &s) {
        if (result.empty()) {
            result = s;
        } else {
            result.push_back(',');
            result += s;
        }
    };
    std::optional<UChar32> last, range_start;
    const auto flush_last = [&]() {
        if (last.has_value()) {
            append(range_start.has_value() && *range_start != *last
                       ? fmt::format("U+{:X}-{:X}", *range_start, *last)
                       : fmt::format("U+{:X}", *last));
            range_start.reset();
        }
    };
    for (const auto c : sorted_codepoints) {
        if (last.has_value() && c - *last != 1) {
            flush_last();
        }
        if (!range_start.has_value()) {
            range_start = c;
        }
        last = c;
    }
    flush_last();
    return result;
}

/**
 * Generates a @font-face CSS rule.
 *
 * \param woff_src The URL to the WOFF2 font
 * \param sorted_unicode_range A span of codepoints to include in the font
 * \param css_kv Additional CSS key-value pairs
 * \return A @font-face CSS rule that includes exactly the specified codepoints
 */
std::string
generate_css(const std::string &woff_src,
             std::span<const UChar32> sorted_unicode_range,
             const std::map<std::string, std::string> &css_kv = {}) {
    using namespace ranges;
    return fmt::format("@font-face {{\n"
                       "  src: url(\"{}\");\n"
                       "  unicode-range: {};\n"
                       "{}}}\n",
                       woff_src, generate_unicode_range(sorted_unicode_range),
                       css_kv | views::transform([](const auto &pair) {
                           return fmt::format("  {}: {};\n", pair.first,
                                              pair.second);
                       }) | views::join |
                           to<std::string>);
}

/**
 * Subsets a font with hb-subset to only include the specified codepoints.
 *
 * \param face The font face to subset
 * \param codepoints A span of codepoints to include in the subset
 */
std::vector<uint8_t> subset_font(hb_face_t *face,
                                 std::span<const UChar32> codepoints) {
    const SubsetInputPtr input{};
    hb_set_t *const unicode_set = hb_subset_input_unicode_set(input.get());
    for (const auto &codepoint : codepoints) {
        hb_set_add(unicode_set, codepoint);
    }
    const FacePtr subsetted{hb_subset_or_fail(face, input.get())};
    const BlobPtr subset_blob{hb_face_reference_blob(subsetted.get())};

    unsigned int uncompressed_length = 0;
    const uint8_t *const uncompressed_data =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t *>(
            hb_blob_get_data(subset_blob.get(), &uncompressed_length));
    const auto compressed_max_length =
        woff2::MaxWOFF2CompressedSize(uncompressed_data, uncompressed_length);
    std::vector<uint8_t> compressed_data(compressed_max_length);
    size_t compressed_length = compressed_max_length;
    woff2::ConvertTTFToWOFF2(uncompressed_data, uncompressed_length,
                             compressed_data.data(), &compressed_length);
    compressed_data.resize(compressed_length);
    return compressed_data;
}

/**
 * Returns the path to the system's temporary directory.
 *
 * \return The path to the system's temporary directory
 */
std::filesystem::path get_temp_dir() {
#if defined(_WIN32) || defined(_WIN64)
    const char *temp_dir = std::getenv("TEMP");
    if (temp_dir == nullptr) {
        temp_dir = std::getenv("TMP");
    }
    if (temp_dir == nullptr) {
        throw std::runtime_error("could not find temp dir");
    }
#else
    const char *temp_dir = std::getenv("TMPDIR");
    if (temp_dir == nullptr) {
        temp_dir = "/tmp";
    }
#endif
    return temp_dir;
}

CostModel build_cost_model_from_data(
    const std::vector<std::pair<size_t, double>> &raw_data) {
    const auto linear = FontLinearCostModel{raw_data};
    spdlog::info("approximate linear cost model: y = {:.2f}x + {:.2f}",
                 linear.cost_per_glyph, linear.cost_base);
    return FontEmpiricalCostModel{raw_data};
}

CostModel build_cost_model(hb_face_t *face, std::span<const UChar32> codepoints,
                           unsigned long rng_seed, int n_samples) {
    const std::filesystem::path cache_path = [&]() {
        // Some quick and dirty hash function to generate a unique identifier
        // for the parameters for this run
        constexpr uint64_t FNV1A_HASH_BASIS = 14695981039346656037ULL;
        constexpr uint64_t FNV1A_HASH_PRIME = 1099511628211ULL;

        const BlobPtr blob{hb_face_reference_blob(face)};
        unsigned int length = 0;
        const char *const blob_data = hb_blob_get_data(blob.get(), &length);
        const std::span<const char> blob_span{blob_data, length};

        uint64_t fnv1a_hash = FNV1A_HASH_BASIS;
        const auto hash = [&fnv1a_hash](const auto x) {
            fnv1a_hash ^= x;
            fnv1a_hash *= FNV1A_HASH_PRIME;
        };
        for (unsigned int i = 0; i < length; i++)
            hash(blob_span[i]);
        // TODO: think again if codepoints should be hashed
        for (const auto c : codepoints)
            hash(c);
        hash(rng_seed);
        hash(n_samples);
        return get_temp_dir() / fmt::format("optift_{:016X}.json", fnv1a_hash);
    }();

    if (std::filesystem::exists(cache_path) &&
        std::filesystem::is_regular_file(cache_path)) {
        spdlog::info("loading cost model raw data from {}",
                     cache_path.string());
        json j;
        std::ifstream f{cache_path};
        f >> j;
        const std::vector<std::pair<size_t, double>> raw_data =
            j["raw_data"]
                .template get<std::vector<std::pair<size_t, double>>>();
        return build_cost_model_from_data(raw_data);
    }

    std::mt19937_64 rng{rng_seed};
    std::uniform_int_distribution<size_t> size_dist{1, codepoints.size()};
    std::vector<std::vector<UChar32>> samples;
    samples.reserve(n_samples);

    for (int i = 0; i < n_samples; i++) {
        const size_t n = size_dist(rng);
        std::vector<UChar32> sample;
        sample.reserve(n);
        std::sample(codepoints.begin(), codepoints.end(),
                    std::back_inserter(sample), n, rng);
        samples.emplace_back(std::move(sample));
    }

    std::vector<std::pair<size_t, double>> raw_data;
    raw_data.reserve(n_samples);
    std::mutex results_mutex;

    using namespace indicators;
    BlockProgressBar bar{
        option::Start{"|"},
        option::End{"|"},
        option::MaxProgress{n_samples},
        option::BarWidth{80}, // NOLINT(*-magic-numbers)
        option::ShowElapsedTime{true},
        option::ShowRemainingTime{true},
        option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
    };

    tbb::parallel_for_each(
        samples.begin(), samples.end(), [&](std::vector<UChar32> &sample) {
            const std::vector<uint8_t> compressed = subset_font(face, sample);
            {
                std::lock_guard lock{results_mutex};
                raw_data.emplace_back(sample.size(),
                                      static_cast<double>(compressed.size()));
                bar.tick();
            }
        });

    bar.mark_as_completed();

    // Save the raw data to a cache file
    {
        json j;
        j["raw_data"] = raw_data;
        std::ofstream f{cache_path};
        f << j.dump(4) << '\n';
        spdlog::info("saved cost model raw data to {}", cache_path.string());
    }

    return build_cost_model_from_data(raw_data);
}

std::pair<PartitionInstance, std::vector<UChar32>>
create_partition_instance(const Input &input, const std::string &font_path,
                          CostModel cost_model, size_t n_partitions) {
    const std::vector<UChar32> item_to_codepoint =
        input.get_all_codepoints_sorted(font_path);
    const auto styles =
        input.get_styles_with_font_path(font_path) | ranges::to<std::vector>;

    std::unordered_map<UChar32, size_t> codepoint_to_item;
    for (size_t i = 0; i < item_to_codepoint.size(); i++) {
        codepoint_to_item[item_to_codepoint[i]] = i;
    }

    PartitionInstance instance{
        .n_partitions = n_partitions,
        .n_items = item_to_codepoint.size(),
        .cost_model = cost_model,
    };

    for (const auto &[_, post] : input.posts) {
        std::unordered_set<size_t> request;

        for (const auto &style : styles) {
            if (const auto it = post.codepoints.find(style);
                it != post.codepoints.end()) {
                icu::StringCharacterIterator it_ch{it->second};
                for (it_ch.setToStart(); it_ch.hasNext();) {
                    const auto c = it_ch.next32PostInc();
                    assert(codepoint_to_item.contains(c));
                    request.insert(codepoint_to_item[c]);
                }
            }
        }

        if (!request.empty()) {
            instance.requests.emplace_back(post.weight, std::move(request));
        }
    }
    // Normalize weights
    const double total_weight = ranges::accumulate(
        instance.requests, 0.0,
        [](double acc, const auto &req) { return acc + req.first; });
    for (auto &[weight, _] : instance.requests) {
        weight /= total_weight;
    }
    return {instance, item_to_codepoint};
}

template <typename T> std::string pretty_print_size(T size_) {
    constexpr double KB = 1024;
    const double size = static_cast<double>(size_);
    if (size < KB) {
        return fmt::format("{:7.2f}  B", size);
    } else if (size < KB * KB) {
        return fmt::format("{:7.2f} KB", size / KB);
    } else if (size < KB * KB * KB) {
        return fmt::format("{:7.2f} MB", size / (KB * KB));
    } else {
        return fmt::format("{:7.2f} GB", size / (KB * KB * KB));
    }
}

/**
 * Compresses a string using gzip.
 *
 * \param data The string to compress
 * \return The compressed data
 */
std::vector<uint8_t> gzip_string(const std::string &data) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error(
            "Failed to initialize zlib for gzip compression");
    }

    stream.avail_in = data.size();
    stream.next_in =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-*-cast)
        reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));

    constexpr size_t BUFFER_SIZE = 1 << 16;
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    std::vector<uint8_t> compressed;

    do { // NOLINT(*-avoid-do-while)
        stream.avail_out = buffer.size();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-*-cast)
        stream.next_out = reinterpret_cast<Bytef *>(buffer.data());

        int ret = deflate(&stream, Z_FINISH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            deflateEnd(&stream);
            throw std::runtime_error("Error during gzip compression");
        }
        const auto compressed_size = buffer.size() - stream.avail_out;
        compressed.insert(
            compressed.end(), buffer.begin(),
            // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
            buffer.begin() + compressed_size);
    } while (stream.avail_out == 0);

    deflateEnd(&stream);
    return compressed;
}

std::vector<std::map<std::string, std::string>>
get_incompatible_styles(const Input &input, const std::string &font_path) {
    using namespace ranges;
    std::unordered_set<std::string> seen;
    std::vector<std::map<std::string, std::string>> result;
    for (const auto &[_, font_spec] : input.fonts) {
        if (font_spec.path != font_path) {
            continue;
        }
        const std::string key =
            font_spec.css | views::transform([](const auto &pair) {
                return fmt::format("{}:{};", pair.first, pair.second);
            }) |
            views::join | to<std::string>;
        if (!seen.contains(key)) {
            seen.insert(key);
            result.push_back(font_spec.css);
        }
    }
    return result;
}

FontPartitionSoln FontPartitionSoln::from_partition_soln(
    const Input &input, const std::string &font_path, hb_face_t *face,
    const PartitionInstance &instance, const PartitionSoln &soln,
    std::span<const UChar32> item_to_codepoint) {
    using namespace ranges;

    const auto map = [](auto &mapping) {
        return views::transform([&mapping](auto i) { return mapping[i]; });
    };

    // Use stem of font path as output base
    // Just a hack for now, should be configurable
    const std::string output_base =
        std::filesystem::path{font_path}.stem().string();

    // Generate font subsets in parallel
    std::vector<std::pair<std::string, std::vector<uint8_t>>> subsetted_fonts(
        soln.partitions.size());
    tbb::parallel_for(size_t(0), soln.partitions.size(), [&](size_t i) {
        if (soln.partitions[i].empty()) {
            return; // Skip empty partitions
        }
        const auto codepoints =
            soln.partitions[i] | map(item_to_codepoint) | to<std::vector>;
        const std::vector<uint8_t> subsetted_font =
            subset_font(face, codepoints);
        subsetted_fonts[i] = {fmt::format("{}-{:02}.woff2", output_base, i),
                              subsetted_font};
    });

    // Generate css
    std::string css = "";
    std::unordered_map<UChar32, size_t> codepoints_to_partition;

    // Find a vector of "incompatible" styles. Different
    const std::vector<std::map<std::string, std::string>> styles_css =
        get_incompatible_styles(input, font_path);

    for (size_t i = 0; i < soln.partitions.size(); i++) {
        if (soln.partitions[i].empty()) {
            continue;
        }
        std::vector<UChar32> codepoints =
            soln.partitions[i] | map(item_to_codepoint) | to<std::vector>;
        ranges::sort(codepoints);
        for (const auto c : codepoints) {
            codepoints_to_partition[c] = i;
        }
        const auto font_output_path =
            fmt::format("./{}", subsetted_fonts[i].first);
        for (const auto &css_kvs : styles_css) {
            css += generate_css(font_output_path, codepoints, css_kvs);
        }
    }

    // Remove empty subsets
    std::erase_if(subsetted_fonts,
                  [](const auto &pair) { return pair.second.empty(); });

    return {css, subsetted_fonts, codepoints_to_partition};
}

void save_and_evaluate_solution(const Input &input,
                                const std::string &font_path, hb_face_t *face,
                                const PartitionInstance &instance,
                                const PartitionSoln &partition_soln,
                                std::span<const UChar32> item_to_codepoint,
                                const argparse::ArgumentParser &program) {
    const std::filesystem::path output_path{
        program.get<std::string>("--output")};

    tbb::task_group g;

    std::optional<FontPartitionSoln> baseline_soln;
    std::optional<FontPartitionSoln> soln_google_fonts;

    if (program.get<bool>("--compare-baseline")) {
        g.run([&] {
            baseline_soln = FontPartitionSoln::from_partition_soln(
                input, font_path, face, instance,
                partition_solve_baseline(instance), item_to_codepoint);
        });
    }
    if (program.get<bool>("--compare-google")) {
        g.run([&] {
            soln_google_fonts =
                FontPartitionSoln::from_google_fonts(input, font_path, face);
        });
    }
    FontPartitionSoln soln = FontPartitionSoln::from_partition_soln(
        input, font_path, face, instance, partition_soln, item_to_codepoint);
    g.wait();

    for (const auto &[filename, subsetted_font] : soln.subsetted_fonts) {
        const auto font_output_path = output_path / filename;
        std::ofstream f{font_output_path, std::ios::binary};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        f.write(reinterpret_cast<const char *>(subsetted_font.data()),
                // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
                subsetted_font.size());
    }
    {
        const auto css_output_path = output_path / "font.css";
        std::ofstream f{css_output_path, std::ios::app};
        f << soln.css;
    }

    using namespace ranges;

    const auto map = [](auto &mapping) {
        return views::transform([&mapping](auto i) { return mapping[i]; });
    };

    const auto compute_total_cost = [&](FontPartitionSoln &soln) {
        return accumulate(
            instance.requests | views::transform([&](const auto &req) {
                const auto &[weight, items] = req;
                const std::unordered_set<size_t> partitions =
                    items | map(item_to_codepoint) |
                    map(soln.codepoint_to_partition) | to<std::unordered_set>;
                const auto subset_size = [&](size_t i) {
                    return soln.subsetted_fonts[i].second.size();
                };
                return weight *
                       accumulate(partitions | views::transform(subset_size),
                                  0.0);
            }),
            0.0);
    };

    const double total_cost = compute_total_cost(soln);
    const double total_cost_with_css =
        total_cost + static_cast<double>(gzip_string(soln.css).size());

    if (baseline_soln.has_value()) {
        const double predicted_cost = instance.eval(partition_soln);
        const double baseline_subset_size = static_cast<double>(
            baseline_soln->subsetted_fonts[0].second.size());

        {
            const double reduction = (baseline_subset_size - total_cost) /
                                     baseline_subset_size * 100.0;
            spdlog::info("total cost predicted       : {}",
                         pretty_print_size(predicted_cost));
            spdlog::info("total cost                 : {} down from {} "
                         "({:.2f}% reduction)",
                         pretty_print_size(total_cost),
                         pretty_print_size(baseline_subset_size), reduction);
        }
        // With CSS (gzipped)
        {
            const double baseline_cost_with_css =
                baseline_subset_size +
                static_cast<double>(gzip_string(baseline_soln->css).size());
            const double reduction =
                (baseline_cost_with_css - total_cost_with_css) /
                baseline_cost_with_css * 100.0;
            spdlog::info("total cost w/ CSS (gzipped): {} down from {} "
                         "({:.2f}% reduction)",
                         pretty_print_size(total_cost_with_css),
                         pretty_print_size(baseline_cost_with_css), reduction);
        }
    } else {
        spdlog::info("total cost: {}", pretty_print_size(total_cost));
        spdlog::info("total cost w/ CSS (gzipped): {}",
                     pretty_print_size(total_cost_with_css));
    }

    if (soln_google_fonts.has_value()) {
        const double total_cost_google_fonts_with_css =
            compute_total_cost(*soln_google_fonts) +
            static_cast<double>(gzip_string(soln_google_fonts->css).size());
        const double reduction =
            (total_cost_google_fonts_with_css - total_cost_with_css) /
            total_cost_google_fonts_with_css * 100.0;
        spdlog::info("total cost vs Google Fonts : {} down from {} "
                     "({:.2f}% reduction)",
                     pretty_print_size(total_cost_with_css),
                     pretty_print_size(total_cost_google_fonts_with_css),
                     reduction);
    }
}

std::optional<std::string>
get_font_name(hb_face_t *face,
              hb_ot_name_id_t name_id = HB_OT_NAME_ID_FULL_NAME) {
    unsigned int name_buffer_size = 0;
    const auto name_size = hb_ot_name_get_utf8(
        face, name_id, HB_LANGUAGE_INVALID, &name_buffer_size, nullptr);
    if (name_size > 0) {
        name_buffer_size = name_size + 1; // Include null terminator
        std::string name(name_buffer_size, '\0');
        hb_ot_name_get_utf8(face, name_id, HB_LANGUAGE_INVALID,
                            &name_buffer_size, name.data());
        return name;
    }
    return std::nullopt;
}

FontPartitionSoln
FontPartitionSoln::from_google_fonts(const Input &input,
                                     const std::string &font_path,
                                     hb_face_t *face, bool subset) {

#include "../eval/google_fonts_baseline.inc"

    const std::string output_base =
        std::filesystem::path{font_path}.stem().string();
    const auto all_codepoints = input.get_all_codepoints_sorted(font_path);

    std::vector<std::vector<UChar32>> partitions(
        GOOGLE_FONTS_PARTITIONS.size());
    std::vector<std::pair<std::string, std::vector<uint8_t>>> subsetted_fonts(
        GOOGLE_FONTS_PARTITIONS.size());

    tbb::parallel_for(size_t(0), GOOGLE_FONTS_PARTITIONS.size(), [&](size_t i) {
        partitions[i] = subset
                            ? ranges::views::set_intersection(
                                  all_codepoints, GOOGLE_FONTS_PARTITIONS[i]) |
                                  ranges::to<std::vector>
                            : GOOGLE_FONTS_PARTITIONS[i];
        if (partitions[i].empty()) {
            return;
        }
        const std::vector<uint8_t> subsetted_font =
            subset_font(face, partitions[i]);
        subsetted_fonts[i] = {fmt::format("{}-{:02}.woff2", output_base, i),
                              subsetted_font};
    });
    std::string css = "";
    std::unordered_map<UChar32, size_t> codepoints_to_partition;

    const std::vector<std::map<std::string, std::string>> styles_css =
        get_incompatible_styles(input, font_path);

    for (size_t i = 0; i < GOOGLE_FONTS_PARTITIONS.size(); i++) {
        for (const auto c : partitions[i]) {
            codepoints_to_partition[c] = i;
        }
        const auto font_output_path =
            fmt::format("./{}", subsetted_fonts[i].first);
        for (const auto &css_kvs : styles_css) {
            css += generate_css(font_output_path, partitions[i], css_kvs);
        }
    }

    return {css, subsetted_fonts, codepoints_to_partition};
}