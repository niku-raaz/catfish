#!/usr/bin/env node
import { mkdir } from "node:fs/promises";
import path from "node:path";
import { spawn } from "node:child_process";
import {
  engineTestingRoot,
  quoteArg,
  resolveBinary,
  resolveCatfishUci,
  resultsDir,
} from "./paths.mjs";

async function main() {
  const catfish = await resolveCatfishUci();
  const stockfish = await resolveBinary("STOCKFISH", "Stockfish");
  const openings =
    process.env.OPENINGS ??
    path.join(engineTestingRoot(), "openings", "smoke-2moves.epd");
  const fastchess = process.env.FASTCHESS ?? "fastchess";
  const seed = process.env.ENGINE_TESTING_SEED ?? "1";
  const pgnOut = path.join(resultsDir(), "stockfish-smoke.pgn");

  await mkdir(resultsDir(), { recursive: true });

  const args = [
    "-engine",
    `cmd=${catfish}`,
    "name=Catfish",
    "option.OwnBook=false",
    "-engine",
    `cmd=${stockfish}`,
    "name=Stockfish",
    "option.Threads=1",
    "-each",
    "depth=4",
    "option.Hash=32",
    "-openings",
    `file=${openings}`,
    "format=epd",
    "order=random",
    "-srand",
    seed,
    "-rounds",
    "20",
    "-repeat",
    "-concurrency",
    "1",
    "-pgnout",
    `file=${pgnOut}`,
    "notation=san",
  ];

  console.log(`${fastchess} ${args.map(quoteArg).join(" ")}`);
  console.log(
    "Wiring-only smoke: depth equality is not strength equality. OwnBook is off.",
  );

  const child = spawn(fastchess, args, {
    stdio: "inherit",
    shell: false,
    cwd: engineTestingRoot(),
  });

  const code = await new Promise((resolve) => {
    child.on("error", (error) => {
      console.error(
        `Failed to launch Fastchess (${fastchess}). ` +
          `Install from https://github.com/Disservin/fastchess or set FASTCHESS.\n` +
          error.message,
      );
      resolve(1);
    });
    child.on("exit", (exitCode) => resolve(exitCode ?? 1));
  });
  process.exitCode = code;
}

main().catch((error) => {
  console.error(error.message);
  process.exitCode = 1;
});
