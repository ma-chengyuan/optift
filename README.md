# OptIFT: Optimizer for Incremental Font Transfer

![Builder](https://github.com/ma-chengyuan/optift/actions/workflows/main.yaml/badge.svg)
![MIT License](https://img.shields.io/badge/license-MIT-blue)

中文版请见 [README.zh_CN.md](README.zh_CN.md).

**TL;DR:** OptIFT minimizes CJK web font loading times by using clever font subsetting and partitioning based on page-wise character usage patterns. It reduces the total bytes loaded by over **50%** compared to Google Fonts.

OptIFT applied to [Vue docs](https://github.com/vuejs-translations/docs-zh-cn):
![Vue](https://quickchart.io/chart?height=400&c=%7Btype%3A%22bar%22%2Coptions%3A%7Bplugins%3A%7Btitle%3A%7Bdisplay%3Atrue%2Ctext%3A%22Font%20size%20reduction%22%7D%7D%2Cscales%3A%7BxAxes%3A%5B%7Bticks%3A%7BautoSkip%3Afalse%2CmaxRotation%3A0%2CminRotation%3A0%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Font%20family%22%7D%7D%5D%2CyAxes%3A%5B%7Bticks%3A%7BbeginAtZero%3Atrue%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Avg%20bytes%20loaded%20on%20uncached%20page%20visit%20(MB)%22%7D%7D%5D%7D%7D%2Cdata%3A%7Blabels%3A%5B%22Noto%20Sans%22%2C%22Noto%20Serif%22%2C%22Source%20Han%20Sans%22%2C%22Smiley%20Sans%22%2C%22Lxgw%20Wen%20Kai%22%5D%2Cdatasets%3A%5B%7Blabel%3A%22Google%20Fonts%22%2Cdata%3A%5B0.511396484375%2C0.9285058593750001%2C1.09298828125%2C0.338955078125%2C0.575517578125%5D%2CbackgroundColor%3A%22rgb(66%2C%20133%2C%20244)%22%7D%2C%7Blabel%3A%22OptIFT%20no%20partitioning%22%2Cdata%3A%5B0.23216796875%2C0.421240234375%2C0.512099609375%2C0.160087890625%2C0.273466796875%5D%2CbackgroundColor%3A%22rgb(244%2C%20180%2C%200)%22%7D%2C%7Blabel%3A%22OptIFT%2010%20partitions%22%2Cdata%3A%5B0.19374023437500001%2C0.343359375%2C0.423359375%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(52%2C%20211%2C%20153)%22%7D%2C%7Blabel%3A%22OptIFT%2015%20partitions%22%2Cdata%3A%5B0.18913085937500002%2C0.33294921875%2C0.409453125%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(16%2C%20185%2C%20129)%22%7D%2C%7Blabel%3A%22OptIFT%2020%20partitions%22%2Cdata%3A%5B0.1869140625%2C0.3301953125%2C0.4048046875%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(5%2C%20150%2C%20105)%22%7D%2C%7Blabel%3A%22OptIFT%2025%20partitions%22%2Cdata%3A%5B0.186396484375%2C0.32894531250000003%2C0.405283203125%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(4%2C%20136%2C%2087)%22%7D%5D%7D%7D)

OptIFT applied to [React docs](https://github.com/reactjs/zh-hans.react.dev):
![React](https://quickchart.io/chart?height=400&c=%7Btype%3A%22bar%22%2Coptions%3A%7Bplugins%3A%7Btitle%3A%7Bdisplay%3Atrue%2Ctext%3A%22Font%20size%20reduction%22%7D%7D%2Cscales%3A%7BxAxes%3A%5B%7Bticks%3A%7BautoSkip%3Afalse%2CmaxRotation%3A0%2CminRotation%3A0%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Font%20family%22%7D%7D%5D%2CyAxes%3A%5B%7Bticks%3A%7BbeginAtZero%3Atrue%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Avg%20bytes%20loaded%20on%20uncached%20page%20visit%20(MB)%22%7D%7D%5D%7D%7D%2Cdata%3A%7Blabels%3A%5B%22Noto%20Sans%22%2C%22Noto%20Serif%22%2C%22Source%20Han%20Sans%22%2C%22Smiley%20Sans%22%2C%22Lxgw%20Wen%20Kai%22%5D%2Cdatasets%3A%5B%7Blabel%3A%22Google%20Fonts%22%2Cdata%3A%5B0.6105859375%2C1.118359375%2C1.322216796875%2C0.3580078125%2C0.61107421875%5D%2CbackgroundColor%3A%22rgb(66%2C%20133%2C%20244)%22%7D%2C%7Blabel%3A%22OptIFT%20no%20partitioning%22%2Cdata%3A%5B0.3226953125%2C0.58626953125%2C0.72220703125%2C0.207109375%2C0.355166015625%5D%2CbackgroundColor%3A%22rgb(244%2C%20180%2C%200)%22%7D%2C%7Blabel%3A%22OptIFT%2010%20partitions%22%2Cdata%3A%5B0.298837890625%2C0.51771484375%2C0.632666015625%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(52%2C%20211%2C%20153)%22%7D%2C%7Blabel%3A%22OptIFT%2015%20partitions%22%2Cdata%3A%5B0.281201171875%2C0.508017578125%2C0.61544921875%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(16%2C%20185%2C%20129)%22%7D%2C%7Blabel%3A%22OptIFT%2020%20partitions%22%2Cdata%3A%5B0.27759765625%2C0.5062695312500001%2C0.6034765625%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(5%2C%20150%2C%20105)%22%7D%2C%7Blabel%3A%22OptIFT%2025%20partitions%22%2Cdata%3A%5B0.27666015625%2C0.5016796875%2C0.60017578125%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(4%2C%20136%2C%2087)%22%7D%5D%7D%7D)

## Table of contents

1. [Introduction](#introduction)
2. [Quick start](#quick-start)
3. Detailed usage
   - [Input JSON specification](#input-json-specification)
   - [Output files](#output-files)
4. [Partitions explained](#partitions-explained)
5. [Comparison with other solutions](#comparison-with-other-solutions)
6. [Building from source](#building-from-source)
7. [Legal considerations](#legal-considerations)
8. [Contributing](#contributing)
9. [Acknowledgements](#acknowledgements)

## Introduction

Web fonts are essential for creating a visually appealing web experience. However, for ideographic languages like Chinese, web fonts are significantly larger than those used for alphabetical languages, which can lead to long loading times.

While common alphabetical fonts like Latin cover their entire alphabet in a few dozen glyphs, CJK fonts require thousands of glyphs. A complete Chinese font can easily reach tens of megabytes in size, creating a major bottleneck for web performance.

OptIFT addresses this problem by:

- Subsetting fonts based on actual character usage in your content.
- Partitioning the subsetted fonts to balance performance for single-page and multi-page visits.
- Generating optimized WOFF2 font files and corresponding CSS for your static site.

If you have a static site with CJK content and want to improve font loading performance, OptIFT is designed for you.

## Quick start

Here’s how to get started with OptIFT in a few steps:

### 1. Install dependencies

Make sure you have CMake, a C++ compiler, and Vcpkg installed. Then clone the OptIFT repository:

```bash
$ git clone https://github.com/ma-chengyuan/optift.git
$ cd optift
```

### 2. Build OptIFT

Make sure you have configured the `VCPKG_ROOT` environment variable.

```bash
$ mkdir build && cd build
$ cmake .. --preset vcpkg
$ cmake --build .
```

### 3. Download prebuilt binaries

You can also download prebuilt binaries from the Github Action artifacts in the [repository](https://github.com/ma-chengyuan/optift/actions). The Linux binaries are fully statically linked, so they should work out of the box on most distributions.

### 4. Run OptIFT

Prepare a simple input JSON file (see [Input JSON Specification](#input-json-specification) for details) and run the following command:

```bash
$ ./optift -i input.json -o output/ -n 10
```

This command will generate partitioned WOFF2 font files and a CSS file in the specified output directory.

## Detailed usage

### Input JSON specification

OptIFT requires an input JSON file specifying the fonts and characters used on your website. Here’s a minimal example:

```json
{
  "fonts": {
    "regular": {
      "path": "path/to/SourceHanSansSC-Regular.otf",
      "css": {
        "font-family": "SourceHanSans",
        "font-weight": "normal"
      }
    }
  },
  "posts": {
    "post-1": {
      "weight": 1,
      "codepoints": {
        "regular": "你好世界"
      }
    }
  }
}
```

#### Explanation

- fonts: Defines the fonts you want to subset. Each font entry contains:
  - `path`: Path to the OTF or TTF font file.
  - `css`: `@font-face` CSS properties to be added in the output CSS.
- posts: Lists pages or posts on your site and their character usage.
  - `weight`: A relative importance weight for the page.
  - `codepoints`: Characters used in the page, grouped by font style.

You can define multiple fonts and posts as needed. For a more complex example, see the full input JSON example.

### Output files

After running OptIFT, you will get:

1. **Subsetted WOFF2 fonts**: Optimized web fonts partitioned into smaller chunks.
2. **CSS file**: A stylesheet linking the generated fonts and defining their usage.

## Partitions explained

OptIFT balances two extremes when subsetting fonts:

1. **One subset per page**: Minimal bytes loaded for single-page visits, but high total bytes if users visit multiple pages.
2. **One subset for the entire site**: Reusable across pages but results in large initial loads.

By creating **partitions**, OptIFT finds a middle ground:

- Common characters are grouped into shared partitions.
- Rare characters are placed in smaller, page-specific partitions.

Empirical testing shows that **10-30 partitions** typically provide the best trade-off.

## Comparison with other solutions

| Feature                       | OptIFT   | Google Fonts | CNFS | Incremental Font Transfer (IFT) |
| ----------------------------- | -------- | ------------ | ---- | ------------------------------- |
| Based on page-wise glyph usage| Yes      | No           | No   | Yes                             |
| Ease of use                   | Medium   | High         | High | High                            |
| Dynamic site support          | No       | Yes          | Yes  | Yes                             |
| OpenType feature support      | Untested | Full         | Full | Full                            |
| Standardized browser support  | No       | No           | No   | Proposed                        |

### Google Fonts

Google Fonts offers a good out-of-the-box solution for CJK web fonts. It uses machine learning to subset and slice fonts efficiently, providing a substantial performance improvement over default solutions. OptIFT, however, often achieves an additional **50% reduction** in bytes loaded by tailoring the fonts specifically to your site’s content.

### cn-font-split (CNFS)

cn-font-split (CNFS) is a mature tool for font partitioning. Unlike OptIFT, which relies on page-wise character usage, CNFS partitions fonts based on the font file alone, making it suitable for both static and dynamic sites. You can find more information about CNFS on its [GitHub repository](https://github.com/KonghaYao/cn-font-split).

While CNFS is easier to use and supports more OpenType features, OptIFT can achieve better results for static sites by leveraging more specific information.

### Incremental Font Transfer (IFT)

Incremental Font Transfer (IFT) is a proposed [W3C standard](https://www.w3.org/TR/IFT/) designed to allow browsers to incrementally download only the portions of a font that are needed for rendering specific content. This approach would significantly improve font loading performance, particularly for large CJK fonts.

However, IFT is still in the proposal stage and has not yet been implemented in browsers. While it holds promise as the ideal future solution for web font optimization, it remains speculative until officially adopted and supported.

For now, OptIFT offers a practical and efficient alternative for static sites.

## Building from source

OptIFT is written in modern C++ and uses CMake + Vcpkg for dependency management. Follow the steps in [Quick Start](#quick-start) to build it on your platform.

### Why C++?

OptIFT is built in C++ primarily because Harfbuzz, the font subsetting library, has a mature C API that integrates more naturally with C++ than with Rust. While Rust is an attractive option, working directly with Harfbuzz in Rust involves additional overhead due to the need for FFI bindings. Once Google Fonts completes a Rust rewrite of a subsetter with features comparable to `hb-subset`, a full Rust rewrite of OptIFT may be considered.

## Legal considerations

OptIFT should only be used with fonts that permit subsetting or partitioning in their licenses. Common licenses that allow this include:

- [SIL Open Font License](https://scripts.sil.org/OFL)

Always check the font’s license before using OptIFT to avoid potential legal issues.

## Contributing

Contributions are welcome! Feel free to open issues or submit pull requests.

## Acknowledgements

I would like to thank **Mingyang Deng** ([website](https://lambertae.github.io/)) for proposing a heuristic approach to font partitioning, which served as the foundation for OptIFT's partitioning strategy.

## License

OptIFT is licensed under the MIT License. See LICENSE for more details.