import * as path from "jsr:@std/path";
import type { Chart } from "npm:@types/chart.js";
import * as changeCase from "npm:change-case";

import {
  deleteGeneratedFiles,
  fonts,
  generateReactPosts,
  generateVuePosts,
} from "./generate_input.ts";

type Result = {
  sizeAfter: number;
  sizeBefore: number;
  reduction: number;
};

async function main(
  partitions: number,
  input_path: string,
): Promise<{ [key: string]: Result }> {
  const cmd = new Deno.Command(
    path.join(import.meta.dirname!, "../build/optift"),
    {
      args: [
        "--input",
        path.join(import.meta.dirname!, input_path),
        "--output",
        path.join(import.meta.dirname!, "output"),
        "--n-partitions",
        partitions.toString(),
        "--compare-baseline",
        "--compare-google",
      ],
      stdout: "piped",
      stderr: "piped",
    },
  );

  const process = cmd.spawn();

  // Create decoders for stdout and stderr
  const stdoutDecoder = new TextDecoder();
  const stderrDecoder = new TextDecoder();

  // Helper function to stream output without extra newlines
  async function streamOutput(
    stream: ReadableStream<Uint8Array>,
    decoder: TextDecoder,
    write: (text: string) => void,
  ): Promise<string> {
    let leftover = "";
    let total = "";
    for await (const chunk of stream) {
      const text = decoder.decode(chunk);
      total += text;
      const lines = (leftover + text).split(/\r?\n/);
      leftover = lines.pop() ?? "";
      for (const line of lines) {
        write(line + "\n");
      }
    }
    if (leftover) write(leftover);
    return total;
  }

  const [stdout, _stderr] = await Promise.all([
    streamOutput(
      process.stdout,
      stdoutDecoder,
      (text) => Deno.stdout.writeSync(new TextEncoder().encode(text)),
    ),

    streamOutput(
      process.stderr,
      stderrDecoder,
      (text) => Deno.stderr.writeSync(new TextEncoder().encode(text)),
    ),
  ]);

  // Wait for the process to complete
  const { code } = await process.status;
  if (code !== 0) {
    console.error(`Process exited with code ${code}`);
    Deno.exit(code);
  }

  // total cost vs Google Fonts :  381.22 KB down from  825.11 KB (53.80% reduction)
  const pattern =
    /total cost vs Google Fonts :\s+(?<sizeAfter>\d+\.\d+ [KMG]?B)\s+down from\s+(?<sizeBefore>\d+\.\d+ [KMG]?B) \((?<reduction>\d+\.\d+)% reduction\)/g;
  const matches = stdout.matchAll(pattern);
  const reductions: { [key: string]: Result } = {};
  let lastIndex = 0;
  for (const match of matches) {
    const fontPath = Array.from(
      stdout.substring(lastIndex, match.index).matchAll(
        /font path: ([\w\-./\\]+)/g,
      ),
    ).at(-1)?.[1];
    if (!fontPath) {
      console.error("No font path found");
      Deno.exit(1);
    }
    const { sizeAfter, sizeBefore, reduction } = match.groups as {
      sizeAfter: string;
      sizeBefore: string;
      reduction: string;
    };
    const fontName = path.basename(fontPath);
    const parseSize = (size: string) => {
      const [value, unit] = size.split(" ");
      const multipliers = {
        KB: 1024,
        MB: 1024 * 1024,
        GB: 1024 * 1024 * 1024,
        B: 1,
      };
      return parseFloat(value) *
        (multipliers[unit as keyof typeof multipliers] ?? 1);
    };
    reductions[fontName] = {
      sizeAfter: parseSize(sizeAfter),
      sizeBefore: parseSize(sizeBefore),
      reduction: parseFloat(reduction),
    };
    lastIndex = match.index;
  }
  return reductions;
}

if (import.meta.main) {
  await deleteGeneratedFiles();
  await Promise.all([generateReactPosts(), generateVuePosts()]);

  const urls = [];
  for (const input of ["vue", "react"]) {
    const chartConfig: Chart.ChartConfiguration = {
      type: "bar",
      options: {
        plugins: { title: { display: true, text: "Font size reduction" } },
        scales: {
          xAxes: [
            {
              ticks: {
                autoSkip: false,
                maxRotation: 0,
                minRotation: 0,
              },
              scaleLabel: {
                display: true,
                labelString: "Font family",
              },
            },
          ],
          yAxes: [
            {
              ticks: {
                beginAtZero: true,
              },
              scaleLabel: {
                display: true,
                labelString: "Avg bytes loaded on uncached page visit (MB)",
              },
            },
          ],
        },
      },
      data: {
        labels: [],
        datasets: [],
      },
    };
    const partitionsToTry = [1, 10, 15, 20, 25];

    chartConfig.data!.labels = fonts.map(([prefix, _f, _s]) =>
      changeCase.capitalCase(prefix.replace(/-.*$/, "").replace(/sc$/i, ""))
    );
    chartConfig.data!.datasets?.push({
      label: "Google Fonts",
      data: [],
      backgroundColor: "rgb(66, 133, 244)",
    });
    const colors = [
      "rgb(244, 180, 0)",
      "rgb(52, 211, 153)",
      "rgb(16, 185, 129)",
      "rgb(5, 150, 105)",
      "rgb(4, 136, 87)",
    ];
    for (const [idx, partitions] of partitionsToTry.entries()) {
      chartConfig.data!.datasets?.push({
        label: partitions > 1
          ? `OptIFT ${partitions} partitions`
          : "OptIFT no partitioning",
        data: [],
        backgroundColor: colors[idx],
      });
    }

    for (const [_prefix, _format, suffix] of fonts) {
      let idx = 0;
      for (const partitions of partitionsToTry) {
        const results = await main(partitions, `fonts_${input}_${suffix}.json`);
        if (idx === 0) {
          const totalSize = Object.values(results).map((r) => r.sizeBefore)
            .reduce((a, b) => a + b, 0) / 1024 / 1024;
          chartConfig.data!.datasets![idx++].data!.push(totalSize);
        }
        const totalSize = Object.values(results).map((r) => r.sizeAfter)
          .reduce((a, b) => a + b, 0) / 1024 / 1024;
        chartConfig.data!.datasets![idx++].data!.push(totalSize);
      }
    }

    urls.push(
      "https://quickchart.io/chart?height=400&c=" +
        encodeURIComponent(
          JSON.stringify(chartConfig).replace(/"(\w+)"\s*:/g, "$1:"),
        ),
    );
  }
  await deleteGeneratedFiles();
  for (const url of urls) {
    console.log(url);
  }
}
