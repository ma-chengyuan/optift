# OptIFT: 增量式网字加载优化

![Builder](https://github.com/ma-chengyuan/optift/actions/workflows/main.yaml/badge.svg)
![MIT License](https://img.shields.io/badge/license-MIT-blue)

OptIFT 通过基于页面汉字具体使用情况使用启发式算法进行字体子集化和拆分，显著减少静态 CJK 网页字体加载大小，与 Google Fonts 相比，加载量减少超过 50%。

OptIFT 在 [Vue 中文文档](https://github.com/vuejs-translations/docs-zh-cn) 上的效果对比:
![Vue](https://quickchart.io/chart?height=400&c=%7Btype%3A%22bar%22%2Coptions%3A%7Bplugins%3A%7Btitle%3A%7Bdisplay%3Atrue%2Ctext%3A%22Font%20size%20reduction%22%7D%7D%2Cscales%3A%7BxAxes%3A%5B%7Bticks%3A%7BautoSkip%3Afalse%2CmaxRotation%3A0%2CminRotation%3A0%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Font%20family%22%7D%7D%5D%2CyAxes%3A%5B%7Bticks%3A%7BbeginAtZero%3Atrue%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Avg%20bytes%20loaded%20on%20uncached%20page%20visit%20(MB)%22%7D%7D%5D%7D%7D%2Cdata%3A%7Blabels%3A%5B%22Noto%20Sans%22%2C%22Noto%20Serif%22%2C%22Source%20Han%20Sans%22%2C%22Smiley%20Sans%22%2C%22Lxgw%20Wen%20Kai%22%5D%2Cdatasets%3A%5B%7Blabel%3A%22Google%20Fonts%22%2Cdata%3A%5B0.511396484375%2C0.9285058593750001%2C1.09298828125%2C0.338955078125%2C0.575517578125%5D%2CbackgroundColor%3A%22rgb(66%2C%20133%2C%20244)%22%7D%2C%7Blabel%3A%22OptIFT%20no%20partitioning%22%2Cdata%3A%5B0.23216796875%2C0.421240234375%2C0.512099609375%2C0.160087890625%2C0.273466796875%5D%2CbackgroundColor%3A%22rgb(244%2C%20180%2C%200)%22%7D%2C%7Blabel%3A%22OptIFT%2010%20partitions%22%2Cdata%3A%5B0.19374023437500001%2C0.343359375%2C0.423359375%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(52%2C%20211%2C%20153)%22%7D%2C%7Blabel%3A%22OptIFT%2015%20partitions%22%2Cdata%3A%5B0.18913085937500002%2C0.33294921875%2C0.409453125%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(16%2C%20185%2C%20129)%22%7D%2C%7Blabel%3A%22OptIFT%2020%20partitions%22%2Cdata%3A%5B0.1869140625%2C0.3301953125%2C0.4048046875%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(5%2C%20150%2C%20105)%22%7D%2C%7Blabel%3A%22OptIFT%2025%20partitions%22%2Cdata%3A%5B0.186396484375%2C0.32894531250000003%2C0.405283203125%2C0.12078125%2C0.205361328125%5D%2CbackgroundColor%3A%22rgb(4%2C%20136%2C%2087)%22%7D%5D%7D%7D)

OptIFT 在 [React 中文文档](https://github.com/reactjs/zh-hans.react.dev) 上的效果对比:
![React](https://quickchart.io/chart?height=400&c=%7Btype%3A%22bar%22%2Coptions%3A%7Bplugins%3A%7Btitle%3A%7Bdisplay%3Atrue%2Ctext%3A%22Font%20size%20reduction%22%7D%7D%2Cscales%3A%7BxAxes%3A%5B%7Bticks%3A%7BautoSkip%3Afalse%2CmaxRotation%3A0%2CminRotation%3A0%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Font%20family%22%7D%7D%5D%2CyAxes%3A%5B%7Bticks%3A%7BbeginAtZero%3Atrue%7D%2CscaleLabel%3A%7Bdisplay%3Atrue%2ClabelString%3A%22Avg%20bytes%20loaded%20on%20uncached%20page%20visit%20(MB)%22%7D%7D%5D%7D%7D%2Cdata%3A%7Blabels%3A%5B%22Noto%20Sans%22%2C%22Noto%20Serif%22%2C%22Source%20Han%20Sans%22%2C%22Smiley%20Sans%22%2C%22Lxgw%20Wen%20Kai%22%5D%2Cdatasets%3A%5B%7Blabel%3A%22Google%20Fonts%22%2Cdata%3A%5B0.6105859375%2C1.118359375%2C1.322216796875%2C0.3580078125%2C0.61107421875%5D%2CbackgroundColor%3A%22rgb(66%2C%20133%2C%20244)%22%7D%2C%7Blabel%3A%22OptIFT%20no%20partitioning%22%2Cdata%3A%5B0.3226953125%2C0.58626953125%2C0.72220703125%2C0.207109375%2C0.355166015625%5D%2CbackgroundColor%3A%22rgb(244%2C%20180%2C%200)%22%7D%2C%7Blabel%3A%22OptIFT%2010%20partitions%22%2Cdata%3A%5B0.298837890625%2C0.51771484375%2C0.632666015625%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(52%2C%20211%2C%20153)%22%7D%2C%7Blabel%3A%22OptIFT%2015%20partitions%22%2Cdata%3A%5B0.281201171875%2C0.508017578125%2C0.61544921875%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(16%2C%20185%2C%20129)%22%7D%2C%7Blabel%3A%22OptIFT%2020%20partitions%22%2Cdata%3A%5B0.27759765625%2C0.5062695312500001%2C0.6034765625%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(5%2C%20150%2C%20105)%22%7D%2C%7Blabel%3A%22OptIFT%2025%20partitions%22%2Cdata%3A%5B0.27666015625%2C0.5016796875%2C0.60017578125%2C0.20634765625%2C0.351396484375%5D%2CbackgroundColor%3A%22rgb(4%2C%20136%2C%2087)%22%7D%5D%7D%7D)

## 目录

1. [简介](#简介)
2. [快速开始](#快速开始)
3. [详细使用说明](#详细使用说明)
   1. [输入 JSON 规范](#输入-json-规范)
   2. [输出文件](#输出文件)
4. [拆分说明](#拆分说明)
5. [与其他方案的比较](#与其他方案的比较)
   1. [Google Fonts](#google-fonts)
   2. [cn-font-split (CNFS)](#cn-font-split-cnfs)
   3. [增量字体传输 (IFT)](#增量字体传输-ift)
6. [从源码构建](#从源码构建)
7. [法律注意事项](#法律注意事项)
8. [贡献指南](#贡献指南)
9. [致谢](#致谢)
10. [许可证](#许可证)

## 简介

网页字体对创建视觉吸引力强、外观跨平台一致的网站至其重要。然而，中文这样的表意文字体系需要的字形数量远远大于英语。现代汉语具有超过 3000 个常用汉字，故而完善的中文字体需要存储成千上万个字形，其大小常常在 10 MB 量级，极大延长了网页的加载时间。

OptIFT 通过以下方式解决这一问题：

- 基于内容的实际字形使用情况生成字体子集。
- 对生成的子集进行拆分，平衡单页和多页访问的性能。
- 为静态网站生成优化后的 WOFF2 字体文件及对应的 CSS 文件。

如果你拥有包含中文内容的静态网站，并希望改善字体加载性能，请考虑 OptIFT!

## 快速开始

以下是使用 OptIFT 的简要步骤：

### 1. 安装依赖

确保已安装 CMake、C++ 编译器和 Vcpkg。然后克隆 OptIFT 仓库：

```bash
$ git clone https://github.com/ma-chengyuan/optift.git
$ cd optift
```

### 2. 构建 OptIFT

确保已遵照 Vcpkg 的安装指南设置 `VCPKG_ROOT` 环境变量&#x20;

```bash
$ mkdir build && cd build
$ cmake .. --preset vcpkg
$ cmake --build .
```

### 3. 下载预构建二进制文件

你也可以从 [仓库](https://github.com/ma-chengyuan/optift/actions) 中的 Github Action Artifacts 下载预编译的可执行文件。面向 Linux 的可执行文件是完全静态链接的，因此在大多数发行版上应该可以直接使用。

### 4. 运行 OptIFT

准备一个简单的输入 JSON 文件（详情参见[输入 JSON 规范](#输入-json-规范)），并运行以下命令：

```bash
$ ./optift -i input.json -o output/ -n 10
```

此命令将在指定的输出目录中生成分区后的 WOFF2 字体文件和 CSS 文件。

## 详细使用说明

### 输入 JSON 规范

OptIFT 需要一个指定网站使用字体和字符的输入 JSON 文件。以下是一个最小示例：

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

#### 说明

- `fonts`：定义你想要生成子集的字体。每个字体条目包含：
  - `path`：OTF 或 TTF 字体文件的路径。
  - `css`：要添加到输出 CSS 中的 `@font-face`  CSS 属性。
- `posts`：列出你网站上的页面或文章及其字符使用情况。
  - `weight`：页面权重。
  - `codepoints`：页面使用的字符，按字体样式分组。

你可以根据需要定义多个字体和页面。有关更复杂的示例，请参见 eval/generate\_input.ts。

### 输出文件

运行 OptIFT 后，你将得到以下文件：

1. **子集化的 WOFF2 字体文件**：经过优化拆分的网页字体。
2. **CSS 文件**：定义生成的字体及其使用方式的样式表。

## 拆分说明

OptIFT 在字体子集化时平衡了两个极端情况：

1. **为每个页面创建一个精确的子集化字体**：对于单页访问，加载的字体大小最少，但如果用户访问多个页面，由于无法重复利用之前的字体，总加载字节数会很高。
2. **整个站点一个子集**：在多个页面之间可重用，但初始加载较大。

通过将字体拆分成不同分区，OptIFT 找到了一个中间方案：

- 将常见字符分组到若干个共享分区中。
- 将稀有字符放入较小的页面特定分区中。

实验证明，**10-30 个分区**通常能提供最佳的平衡。

## 与其他方案的比较

| 功能            | OptIFT | Google Fonts | CNFS | 增量字体传输 (IFT) |
| ------------- | ------ | ------------ | ---- | ------------ |
| 基于页面字符使用情况      | 是      | 否            | 否    | 是            |
| 易用性           | 中      | 高            | 高    | 高            |
| 动态网站支持        | 否      | 是            | 是    | 是            |
| OpenType 特性支持 | 未测试    | 全部           | 全部   | 全部           |
| 标准化浏览器支持      | 否      | 否            | 否    | 提议           |

### Google Fonts

Google Fonts 为 CJK 网页字体提供了一个开箱即用的优秀解决方案。它使用机器学习来高效地子集化和切片字体，与默认解决方案相比，提供了显著的性能提升。然而，OptIFT 针对特定站点内容定制字体，通常能进一步减少 **50% 的加载量**。

### cn-font-split (CNFS)

cn-font-split (CNFS) 是中文网字计划推出的一个成熟的字体分包工具。与 OptIFT 不同，CNFS 基于字体文件本身进行拆分，故而对动态和静态网页都可用。更多信息请参见 [GitHub 仓库](https://github.com/KonghaYao/cn-font-split) 。

虽然 CNFS 更易于使用并支持更多的 OpenType 特性，但 OptIFT 通过利用每个页面具体的字形使用情况，可以为静态站点带来更好的效果。

### 增量字体传输 (IFT)

增量字体传输 (IFT) 是一个拟议的 [W3C 标准](https://www.w3.org/TR/IFT/)，旨在使浏览器能够逐步下载仅渲染特定内容所需的字体部分。这种方法将显著改善大 CJK 字体的加载性能。

然而，IFT 仍处于提议阶段，尚未在浏览器中实现。尽管它作为未来网页字体优化的理想解决方案充满希望，但在正式采用和支持之前仍是推测性的。

目前，OptIFT 为静态站点提供了一个实用且高效的替代方案。

## 从源码构建

OptIFT 使用 Modern C++ 编写，并使用 CMake + Vcpkg 进行依赖管理。按照 [快速开始](#快速开始) 中的步骤即可在你的平台上构建。

### 为什么选择 C++？

OptIFT 主要使用 C++ 编写，是因为 Harfbuzz 是一个 C 库，与 C++ 的集成比与 Rust 更简单。虽然 Rust 是一个很香的选项，但在 Rust 中直接使用 Harfbuzz 涉及到 FFI 绑定的额外开销。一旦 Google Fonts 完成功能与 `hb-subset` 相当的 Rust 子集化工具，OptIFT 可能会考虑使用 Rust 完全重写。

## 法律注意事项

OptIFT 仅应与许可证中允许二次开发、子集化或分区的字体一起使用。常见允许此操作的许可证包括：

- [SIL Open Font License](https://scripts.sil.org/OFL)

在使用 OptIFT 之前，请务必检查字体的许可证，以避免潜在的法律问题。

## 贡献指南

欢迎 PR！

## 致谢

OptIFT 使用的启发式算法基于某日和 [dmy 老师](https://lambertae.github.io/) 的杂谈。感谢邓老师!

## 许可证

OptIFT 根据 MIT 许可证授权。有关更多详细信息，请参见 LICENSE 文件。

