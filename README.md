# OptIFT: Optimizer for Incremental Font Transfer

**TL;DR:** Clever font subsetting and partitioning for static sites to minimize CJK web font loading times. >50% less bytes loaded compared with Google Fonts.

OptIFT on [Vue docs](https://github.com/vuejs-translations/docs-zh-cn):

![Vue](https://quickchart.io/chart?height=400&c={type:"bar",options:{plugins:{title:{display:true,text:"Font size reduction"}},scales:{xAxes:[{ticks:{autoSkip:false,maxRotation:0,minRotation:0},scaleLabel:{display:true,labelString:"Font family"}}],yAxes:[{ticks:{beginAtZero:true},scaleLabel:{display:true,labelString:"Avg bytes loaded on uncached page visit (MB)"}}]}},data:{labels:["NotoSansSC","NotoSerifSC","OPPOSans","SourceHanSansSC"],datasets:[{label:"Google Fonts",data:[0.511396484375,0.92849609375,0.568837890625,1.092978515625],backgroundColor:"rgb(66, 133, 244)"},{label:"OptIFT no partitioning",data:[0.23216796875,0.421240234375,0.27622070312499997,0.512099609375],backgroundColor:"rgb(244, 180, 0)"},{label:"OptIFT 10 partitions",data:[0.19374023437500001,0.343359375,0.180400390625,0.42337890624999996],backgroundColor:"rgb(52, 211, 153)"},{label:"OptIFT 15 partitions",data:[0.18913085937500002,0.33294921875,0.16615234375,0.40947265625],backgroundColor:"rgb(16, 185, 129)"},{label:"OptIFT 20 partitions",data:[0.1869140625,0.3301953125,0.16450195312500002,0.40481445312500003],backgroundColor:"rgb(5, 150, 105)"},{label:"OptIFT 25 partitions",data:[0.186396484375,0.32894531250000003,0.15898437499999998,0.40529296875000004],backgroundColor:"rgb(4, 136, 87)"}]}})

OptIFT on [React docs](https://github.com/reactjs/zh-hans.react.dev):

![React](https://quickchart.io/chart?height=400&c={type:"bar",options:{plugins:{title:{display:true,text:"Font size reduction"}},scales:{xAxes:[{ticks:{autoSkip:false,maxRotation:0,minRotation:0},scaleLabel:{display:true,labelString:"Font family"}}],yAxes:[{ticks:{beginAtZero:true},scaleLabel:{display:true,labelString:"Avg bytes loaded on uncached page visit (MB)"}}]}},data:{labels:["NotoSansSC","NotoSerifSC","OPPOSans","SourceHanSansSC"],datasets:[{label:"Google Fonts",data:[0.6105859375,1.11833984375,0.6790625,1.322216796875],backgroundColor:"rgb(66, 133, 244)"},{label:"OptIFT no partitioning",data:[0.3226953125,0.58626953125,0.387978515625,0.722216796875],backgroundColor:"rgb(244, 180, 0)"},{label:"OptIFT 10 partitions",data:[0.298837890625,0.51771484375,0.2476953125,0.632666015625],backgroundColor:"rgb(52, 211, 153)"},{label:"OptIFT 15 partitions",data:[0.281201171875,0.508017578125,0.234326171875,0.61544921875],backgroundColor:"rgb(16, 185, 129)"},{label:"OptIFT 20 partitions",data:[0.27759765625,0.5062695312500001,0.2263671875,0.603486328125],backgroundColor:"rgb(5, 150, 105)"},{label:"OptIFT 25 partitions",data:[0.27666015625,0.5016796875,0.21443359375,0.60017578125],backgroundColor:"rgb(4, 136, 87)"}]}})

## What OptIFT solves

Web fonts are cool! But not when they are slow. Alphabetical languages like English have their entire alphabets covered by a few dozen glyphs and a full web font can usually be kept under 100 KB, which loads in an instant given today’s internet speed. The story is entirely different for ideographic languages such as Chinese. There are 3500–4000 Hanzis (Chinese characters) in daily use, and tens of thousands more uncommon ones, each requiring its own glyph. Consequently, decently complete Chinese fonts are around tens of megabytes each. The huge size raises significant barriers of serving them over the web. 

Hope is not all lost, however. While the absolute number of Hanzis is daunting, much less is used in any given piece of writing. Hence web fonts can be made practical if we subset the font to just what we use. OptIFT caters to exactly this use case and deals with many of the intricacies of this idea. In particular, you specify:

1. What fonts you want to use (path to OTF or TTF);
2. What page contains which characters, and in which font;
3. How many partitions you want to use (more on that later); 
4. Optionally, weights of your pages (as you may get from traffic analytics).

OptIFT gives you

1. A set of partitioned subsetted WOFF2 web fonts.
2. A CSS styleheet that you can embed into your site to use these fonts. 

**In general, you may consider OptIFT if**

1. You want to use web fonts for the CJK contents on your site;
2. The CJK contents are relatively static (such as blog posts or technical documentations).

## Usage

```
Usage: optift [--help] [--version] --input VAR --output VAR --n-partitions VAR [--rng VAR] [--samples VAR] [--compare-baseline] [--compare-google]

Optional arguments:
  -h, --help          shows help message and exits 
  -v, --version       prints version information and exits 
  -i, --input         path to the input JSON file [required]
  -o, --output        path to the output directory [required]
  -n, --n-partitions  number of partitions to create [required]
  --rng               RNG seed for sampling cost model [nargs=0..1] [default: 42]
  --samples           number of samples for cost model [nargs=0..1] [default: 100]
  --compare-baseline  compare heuristic solution to baseline solution 
  --compare-google    compare heuristic solution to Google Fonts solution 

```

### JSON input

To use OptIFT, you need an input JSON file that specifies the font to subset and the characters used by your website. It looks something like this:

```json
{
  "fonts": {
    "regular": {
      "path": "/home/chengyuan/Projects/font-splitter-cpp/eval/SourceHanSansSC-Regular.otf",
      "css": {
        "font-family": "NotoSans",
        "font-weight": "normal"
      }
    },
    "bold": {
      "path": "/home/chengyuan/Projects/font-splitter-cpp/eval/SourceHanSansSC-Bold.otf",
      "css": {
        "font-family": "NotoSans",
        "font-weight": "bold"
      }
    }
  },
  "posts": {
    "zh-hans.react.dev-main/src/content/blog/2024/05/22/react-conf-2024-recap.md": {
      "weight": 1,
      "codepoints": {
        "regular": "回顾上周我们在内华达州亨德森举办了为期两天的大会，有多名与者亲临现场讨论工程领域最新进展。这篇文章中将总结次活动演讲和公告年月日是自以来首线下议很高兴能够再社区团一起宣布、架构版及实验本同时登台通用服务器组件等完整第二直播已经可观看由席技术官发表欢迎致辞随后织负责人主持行题分享对目标愿景：帮助任何更加轻松建卓越户体带状报载量超过亿并且开学习编她强调成就今容请查原生节奏鲁斯支介绍接即推出功该准备好于产环境测试你全部也关深入析哪些解读路图协使增单计算机设应理源个供家尝其作信息档如忘吧感谢氛围包括万他还页面；市品详情每访问微软始菜几乎移桌所做库框平不仅空间继续阶段改未跨方创都样启此门指南末尾答队束性错误揭秘代打破规则决员参太长列但想特别倡导策划担提聚讯细反馈幻灯片填补缺网站赞商得音视频觉舞声威汀酒店住宿知识位它到共真鼓心见！",
        "bold": "点击这里观看第一天完整直播。二"
      }
    },
  }
}
```

The root object has two fields:

* `fonts`: an object holding font information. The keys will be the fonts used in your website. The value of each key is an object consisting of the keys below:

  * `path`: path to the font file. For now only OTF and TTF are supported.
  * `css`: CSS [key-value pairs](https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face) to be added to the `@font-face` definitions of the subsetted f'ont in the output CSS. (except `unicode-range`, which will be overwritten by what OptIFT generates anyways) 

  The exact names of the keys does not matter as long as they are consistent with what’s used in the `posts` section below. Even you are using just one typeface, it’s a good idea to create multiple font objects corresponding to its different styles (bold, italic, small caps) and where it’s used (title, heading, body), because (1) different styles may point to different font files and (2) life will be easier if you decide to use multiple fonts later. OptIFT internally merges fonts with the same font file and CSS key-values. 

* `posts`: an object holding which characters are used by which font in which page / post (called `posts` because it was originally designed for static blogs). The keys can be arbitrary and serve no real purpose except for visual inspection. The values are objects with two fields.

  * `weight`: the weight of the page. Roughly speaking, OptIFT optimizes to minimize the weighted average of “page loading cost.” So a page with higher weight (such as index) will be prioritized by OptIFT.
  * `codepoints`: an object keyed by the fonts you defined in the `fonts` section. Values are strings containing the characters styled in that font in the page. Order does not matter; duplications are ignored. As the name hints, OptIFT assumes that *the rendering of every Unicode code point can be done deterministically in isolation* — not true in general, but this assumption holds up nicely in CJK  languages. 

Generation of the JSON file is beyond the scope of this project. It is recommended that you extend your favorite CMS or site generator to programmatically generate this JSON and keep it in sync with your website. `eval/generate_input.ts` shows a simple example of how this can be done. 

### Output

OptIFT requires that you specify an output directory to hold the subsetted fonts and the generated CSS. This can be anywhere but it’s recommended that you put it under the output folder of whatever site generator you are using, since OptIFT should be run whenever the site is built or updated.

### Partitions

OptIFT slices the subsetted font in partitions to balance the bytes loaded in an isolated visit and the bytes loaded across visits to multiple pages on a given site. Consider the following extreme cases:

* A unique subsetted font for every web page makes isolated visits as fast as they can be, but if a user visits many pages, many such fonts would need to be loaded.
* A subsetted font for the entire site makes the font reusable across different pages, but someone who visits a single page risks loading more than it needs. This is especially true if the page most frequent visited is light in content (such as index).

A middle ground solution is to subset the font for the entire site, and further partition it such that each page only loads a subset of all partitions. One could imagine having a few partitions that cover the most common characters used by every post and few that are there only for a few post using some super rare characters. Empirically, having 10–30 partitions seems to be a sweet spot. 

## Comparison with other solutions

### Google Fonts

Google has been using machine learning techniques to optimize slicing of CJK fonts since 2017 — basically doing what this project intends to do on the scale of the entire internet! Therefore, serving CJK fonts from Google Fonts is a quick, no-brainer solution that’s already much much better than serving the full font file and gets you 70% of what’s theoretically achievable. If you can get away with Google Fonts, great!

OptIFT is for those who are not content with being “70% there” or experimenting fonts that are not available on Google Fonts. OptIFT provides a flag (`--compare-google`) that produces a comparison of estimated cost (total bytes loaded) of its output versus Google Fonts’, and empirically OptIFT often wins by ~50%! (for regular style; ~70% is common for bold or italic fonts which are used more sparsely)

### [cn-font-split](https://github.com/KonghaYao/cn-font-split/tree/release/packages/ffi-js)

cn-font-split (“CNFS” below) is a more popular, mature tool in this domain. OptIFT and CNFS are largely **orthogonal and/or complementary.** By default, CNFS partitions fonts solely based on the font file itself (and not how it’s used in your website), using configurable partition schemes (such as one based on Google Font’s). Hence, CNFS works for dynamic sites and static sites alike. In contrast, I built OptIFT to demonstrate that incorporating page-wise character usage admits better partitioning. OptIFT is deliberately more specialized and more algorithmic in nature. 

Things OptIFT and CNFS have in common:

* Both are based on Harfbuzz, which does the heavy lifting of parsing and subsetting OpenType font files.
* Both are parallelized: CNFS uses Rust + Rayon, whereas OptIFT uses TBB.

You may prefer CNFS for 

* better support for OpenType features — OptIFT has no special code other than those in Harfbuzz to handle OT features nor is it tested on these cases. 
* better ease-of-use — CNFS is distributed via NPM, and has Vite plugins available. OptIFT requires more DIY in this regard.

Prefer OptIFT if

* you embrace perfectionism and enjoy seeing things pushed to the extreme: using as much information as possible to make every byte over the wire count — does that sound interesting to you?
* you are willing to take the time to integrate it into your publishing work flow.
* you enjoy taking the road not taken.

Combining OptIFT and CNFS is possible in theory: CNFS provides interface to provide custom partitioning strategies, which could call OptIFT. How that is done is currently out of the scope of this project (PRs are welcome!).