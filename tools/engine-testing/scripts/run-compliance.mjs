#!/usr/bin/env node
import { spawn } from "node:child_process";
import { resolveCatfishUci } from "./paths.mjs";

async function resolveFastchess() {
  const configured = process.env.FASTCHESS ?? "fastchess";
  return configured;
}

async function main() {
  const catfish = await resolveCatfishUci();
  const fastchess = await resolveFastchess();
  console.log(`Running: ${fastchess} --compliance ${catfish}`);

  const child = spawn(fastchess, ["--compliance", catfish], {
    stdio: "inherit",
    shell: false,
  });

  const code = await new Promise((resolve) => {
    child.on("error", (error) => {
      console.error(
        `Failed to launch Fastchess (${fastchess}). Install from ` +
          "https://github.com/Disservin/fastchess and ensure it is on PATH, " +
          `or set FASTCHESS.\n${error.message}`,
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
