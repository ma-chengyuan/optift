import * as path from "jsr:@std/path";
import * as zipJs from "jsr:@zip-js/zip-js";

import rehypeStringify from "npm:rehype-stringify";
import remarkParse from "npm:remark-parse";
import remarkRehype from "npm:remark-rehype";
import remarkGfm from "npm:remark-gfm";
import { unified } from "npm:unified";
import { JSDOM } from "npm:jsdom";

type WeightedPost<T> = { weight: number } & T;
type MarkdownPost = WeightedPost<{ markdown: string }>;
type HTMLPost = WeightedPost<{ html: string }>;
type OptiftPost = WeightedPost<{ codepoints: { [style: string]: string } }>;

const markdownProcessor = unified()
  .use(remarkGfm)
  .use(remarkParse)
  .use(remarkRehype)
  .use(rehypeStringify);

async function markdownToHTML(post: MarkdownPost): Promise<HTMLPost> {
  return {
    weight: post.weight,
    html: String(await markdownProcessor.process(post.markdown)),
  };
}

function htmlToOptift(post: HTMLPost) {
  const dom = new JSDOM(post.html);

  const codepointSet = {} as { [style: string]: Set<string> };

  function walkHTML(
    node: HTMLElement,
    curStyle: string,
    codepoints: { [style: string]: Set<string> }
  ) {
    if (node.nodeType === 3 /* Node.TEXT_NODE */) {
      const cjkRegex = /[\u4e00-\u9fff\u3000-\u303f\uff00-\uffef]/g;
      const text = node.textContent ?? "";
      const set =
        curStyle in codepoints ? codepoints[curStyle] : new Set<string>();
      for (const match of text.match(cjkRegex) ?? []) set.add(match);
      if (set.size > 0) codepoints[curStyle] = set;
    }

    if (node.nodeType === 1 /* Node.ELEMENT_NODE */) {
      const mapping: { [tag: string]: { [from: string]: string } } = {
        EM: {
          regular: "italic",
          bold: "bold-italic",
        },
        STRONG: {
          regular: "bold",
          italic: "bold-italic",
        },
      };
      const childStyle =
        node.tagName in mapping
          ? mapping[node.tagName][curStyle] ?? curStyle
          : curStyle;
      for (const child of node.childNodes) {
        walkHTML(child as HTMLElement, childStyle, codepoints);
      }
    }
  }

  walkHTML(dom.window.document.body, "regular", codepointSet);
  const codepoints = {} as { [style: string]: string };
  for (const style in codepointSet) {
    codepoints[style] = Array.from(codepointSet[style]).join("");
  }
  return {
    weight: post.weight,
    codepoints,
  };
}

async function markdownPostsFromZipRepo(
  url: string,
  prefix: string
): Promise<{ [key: string]: OptiftPost }> {
  const response = await fetch(url);
  const blob = await response.blob();
  const zipReader = new zipJs.ZipReader(new zipJs.BlobReader(blob));
  const ret = {} as { [key: string]: OptiftPost };
  const promises = [];
  for (const entry of await zipReader.getEntries()) {
    if (
      entry.directory ||
      !entry.filename.startsWith(prefix) ||
      !entry.filename.endsWith(".md") ||
      !entry.getData
    ) {
      continue;
    }
    promises.push(
      entry
        .getData(new zipJs.TextWriter())
        .then((data) => markdownToHTML({ weight: 1.0, markdown: data }))
        .then((htmlPost) => {
          ret[entry.filename] = htmlToOptift(htmlPost);
        })
    );
  }
  await Promise.all(promises);
  await zipReader.close();
  return ret;
}

function nextToMe(rel_path: string) {
  return path.join(import.meta.dirname ?? "", rel_path);
}

function attachFonts(posts: { [key: string]: OptiftPost }) {
  const regular = {
    path: nextToMe("SourceHanSansSC-Regular.otf"),
    css: {
      "font-family": "NotoSans",
      "font-weight": "normal",
    },
  };
  const bold = {
    path: nextToMe("SourceHanSansSC-Bold.otf"),
    css: {
      "font-family": "NotoSans",
      "font-weight": "bold",
    },
  };

  return {
    fonts: {
      regular: regular,
      italic: regular,
      bold: bold,
      "bold-italic": bold,
    },
    posts: posts,
  };
}

async function generateVuePosts() {
  const vuePosts = await markdownPostsFromZipRepo(
    "https://github.com/vuejs-translations/docs-zh-cn/archive/refs/heads/main.zip",
    "docs-zh-cn-main/src/"
  );
  await Deno.writeTextFile(
    nextToMe("fonts_vue.json"),
    JSON.stringify(attachFonts(vuePosts), null, 2)
  );
  console.log("Vue posts generated");
}

async function generateReactPosts() {
  const reactPosts = await markdownPostsFromZipRepo(
    "https://github.com/reactjs/zh-hans.react.dev/archive/refs/heads/main.zip",
    "zh-hans.react.dev-main/src/content/"
  );
  await Deno.writeTextFile(
    nextToMe("fonts_react.json"),
    JSON.stringify(attachFonts(reactPosts), null, 2)
  );
  console.log("React posts generated");
}

await Promise.all([generateReactPosts(), generateVuePosts()]);
