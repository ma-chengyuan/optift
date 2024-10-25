import * as path from "jsr:@std/path";

const req = await fetch(
  "https://fonts.googleapis.com/css2?family=Noto+Sans+SC",
  {
    // Need to pretend we are latest Chrome to get properly-subsetted fonts
    headers: new Headers({
      "User-Agent":
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36",
    }),
  }
);
const css = await req.text();
const partitions = css
  .split("\n")
  .filter((line) => line.trimStart().startsWith("unicode-range"))
  .map((unicodeRange) => {
    const ranges = unicodeRange.split(":")[1].split(",");
    return ranges.map((range) =>
      range
        .replace(/\s*U\+/g, "")
        .split("-")
        .map((x) => parseInt(x, 16))
    );
  });
const initializer = partitions
  .map((partition) => {
    const initializer = partition
      .flatMap((range) =>
        range.length === 1
          ? [range[0]]
          : new Array(range[1] - range[0] + 1).map((_, i) => range[0] + i)
      )
      .sort((a, b) => a - b)
      .map((x) => `0x${x.toString(16)}`)
      .join(", ");
    return `{${initializer}}`;
  })
  .join(",\n    ");
const source = `const static std::vector<std::vector<UChar32>> GOOGLE_FONTS_PARTITIONS{\n    ${initializer}\n};`;
const target = path.join(
  import.meta.dirname ?? "",
  "google_fonts_baseline.inc"
);
Deno.writeTextFile(target, source);
