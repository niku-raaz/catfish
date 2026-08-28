#!/usr/bin/env node
/**
 * Process-level UCI transcript tests against the real catfish_uci executable.
 * These deliberately do not use UciSession in-process.
 */
import { spawn } from "node:child_process";
import { resolveCatfishUci } from "./paths.mjs";

class UciProcess {
  constructor(command) {
    this.command = command;
    this.child = null;
    this.stdoutBuffer = "";
    this.stderr = "";
    this.stdoutLines = [];
    this.waiters = new Set();
    this.closed = false;
    this.exitCode = null;
  }

  start() {
    this.child = spawn(this.command, [], {
      stdio: ["pipe", "pipe", "pipe"],
      shell: false,
      windowsHide: true,
    });
    this.child.stdout.setEncoding("utf8");
    this.child.stderr.setEncoding("utf8");
    this.child.stdout.on("data", (chunk) => {
      this.stdoutBuffer += chunk;
      const parts = this.stdoutBuffer.split(/\r?\n/);
      this.stdoutBuffer = parts.pop() ?? "";
      for (const line of parts) {
        this.stdoutLines.push(line);
        for (const waiter of [...this.waiters]) {
          if (waiter.predicate(line, this.stdoutLines)) {
            clearTimeout(waiter.timer);
            this.waiters.delete(waiter);
            waiter.resolve(line);
          }
        }
      }
    });
    this.child.stderr.on("data", (chunk) => {
      this.stderr += chunk;
    });
    this.child.on("error", (error) => {
      this.closed = true;
      for (const waiter of this.waiters) {
        clearTimeout(waiter.timer);
        waiter.reject(error);
      }
      this.waiters.clear();
    });
    this.child.on("exit", (code) => {
      this.closed = true;
      this.exitCode = code;
      for (const waiter of this.waiters) {
        clearTimeout(waiter.timer);
        waiter.reject(new Error(`engine exited with code ${code}`));
      }
      this.waiters.clear();
    });
  }

  write(command) {
    if (!this.child || this.closed) {
      throw new Error("engine is not running");
    }
    this.child.stdin.write(`${command}\n`);
  }

  waitFor(predicate, timeoutMs = 10_000) {
    for (const line of this.stdoutLines) {
      if (predicate(line, this.stdoutLines)) {
        return Promise.resolve(line);
      }
    }
    return new Promise((resolve, reject) => {
      const waiter = {
        predicate,
        resolve,
        reject,
        timer: setTimeout(() => {
          this.waiters.delete(waiter);
          reject(new Error(`timeout after ${timeoutMs}ms waiting for UCI output`));
        }, timeoutMs),
      };
      this.waiters.add(waiter);
    });
  }

  async quit() {
    if (!this.child || this.closed) {
      return;
    }
    this.write("quit");
    await new Promise((resolve) => {
      const timer = setTimeout(() => {
        this.child.kill();
        resolve();
      }, 2_000);
      this.child.once("exit", () => {
        clearTimeout(timer);
        resolve();
      });
    });
  }
}

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

async function runCase(name, fn) {
  process.stdout.write(`  ${name} ... `);
  await fn();
  process.stdout.write("ok\n");
}

async function main() {
  const enginePath = await resolveCatfishUci();
  console.log(`UCI process tests using ${enginePath}`);

  await runCase("uci → uciok handshake", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    const joined = engine.stdoutLines.join("\n");
    assert(joined.includes("id name Catfish"), "missing id name");
    assert(joined.includes("option name Hash"), "missing Hash option");
    await engine.quit();
    assert(engine.exitCode === 0 || engine.exitCode === null, "non-zero exit");
  });

  await runCase("isready → readyok", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    engine.write("isready");
    await engine.waitFor((line) => line === "readyok");
    await engine.quit();
  });

  await runCase("options accepted only after handshake", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("setoption name Hash value 16");
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    engine.write("setoption name Hash value 16");
    engine.write("isready");
    await engine.waitFor((line) => line === "readyok");
    await engine.quit();
  });

  await runCase("multiple games in one process", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    engine.write("setoption name OwnBook value false");
    engine.write("ucinewgame");
    engine.write("position startpos");
    engine.write("go depth 1");
    const first = await engine.waitFor((line) => line.startsWith("bestmove "));
    assert(/^bestmove ([a-h][1-8][a-h][1-8][qrbn]?|0000)$/u.test(first), first);
    engine.write("ucinewgame");
    engine.write("position startpos moves e2e4 e7e5");
    engine.write("go depth 1");
    const second = await engine.waitFor((line) => line.startsWith("bestmove "));
    assert(/^bestmove ([a-h][1-8][a-h][1-8][qrbn]?|0000)$/u.test(second), second);
    await engine.quit();
  });

  await runCase("transactional invalid positions", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    engine.write("setoption name OwnBook value false");
    engine.write("position startpos");
    engine.write("go depth 1");
    await engine.waitFor((line) => line.startsWith("bestmove "));
    const beforeStderr = engine.stderr;
    engine.write("position startpos moves e2e5");
    engine.write("isready");
    await engine.waitFor((line) => line === "readyok");
    assert(
      engine.stderr.length > beforeStderr.length ||
        engine.stderr.includes("illegal"),
      "expected diagnostic for illegal move on stderr",
    );
    engine.write("position startpos");
    engine.write("go depth 1");
    const move = await engine.waitFor((line) => line.startsWith("bestmove "));
    assert(move.startsWith("bestmove "), move);
    await engine.quit();
  });

  await runCase("terminal bestmove 0000", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    engine.write("setoption name OwnBook value false");
    engine.write("position fen 7k/5Q2/7K/8/8/8/8/8 b - - 0 1");
    engine.write("go depth 1");
    const line = await engine.waitFor((line) => line.startsWith("bestmove "));
    assert(line === "bestmove 0000", `expected bestmove 0000, got ${line}`);
    await engine.quit();
  });

  await runCase("no diagnostics on stdout", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    engine.write("setoption name OwnBook value false");
    engine.write("position startpos moves e2e5");
    engine.write("isready");
    await engine.waitFor((line) => line === "readyok");
    for (const line of engine.stdoutLines) {
      assert(
        !/illegal|error|warn|exception|assert/iu.test(line) ||
          line.startsWith("id ") ||
          line.startsWith("option ") ||
          line === "uciok" ||
          line === "readyok",
        `unexpected diagnostic-like stdout line: ${line}`,
      );
    }
    await engine.quit();
  });

  await runCase("unknown info tokens tolerated by controller parse", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    engine.write("setoption name OwnBook value false");
    engine.write("position startpos");
    engine.write("go depth 1");
    await engine.waitFor((line) => line.startsWith("bestmove "));
    const info = engine.stdoutLines.find((line) => line.startsWith("info depth "));
    assert(info, "expected an info line");
    // Controllers ignore unknown tokens; Catfish may emit qnodes/hashhits.
    assert(/\bqnodes\b/u.test(info) || /\bnodes\b/u.test(info), info);
    await engine.quit();
  });

  await runCase("clean quit", async () => {
    const engine = new UciProcess(enginePath);
    engine.start();
    engine.write("uci");
    await engine.waitFor((line) => line === "uciok");
    await engine.quit();
    assert(engine.closed, "engine did not exit");
    assert(
      engine.exitCode === 0 || engine.exitCode === null,
      `unexpected exit code ${engine.exitCode}`,
    );
  });

  console.log("All UCI process tests passed.");
}

main().catch((error) => {
  console.error(`\nFAILED: ${error.message}`);
  process.exitCode = 1;
});
