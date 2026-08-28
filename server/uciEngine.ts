import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { access } from "node:fs/promises";
import path from "node:path";
import type { EngineInfo, SearchResponse } from "./contracts.js";

type LineWaiter = {
  predicate: (line: string) => boolean;
  resolve: (line: string) => void;
  reject: (error: Error) => void;
  timer: NodeJS.Timeout;
};

type EngineOptions = {
  command?: string;
  args?: string[];
  timeoutMs?: number;
  hashSizeMb?: number;
  maxPendingSearches?: number;
};

const UCI_MOVE_PATTERN = /^[a-h][1-8][a-h][1-8][qrbn]?$/;

function boundedInteger(
  value: string | undefined,
  fallback: number,
  minimum: number,
  maximum: number,
): number {
  const parsed = Number(value);
  return Number.isInteger(parsed) && parsed >= minimum && parsed <= maximum
    ? parsed
    : fallback;
}

export class LineBuffer {
  private buffer = "";

  push(chunk: string): string[] {
    this.buffer += chunk;
    const lines = this.buffer.split(/\r?\n/);
    this.buffer = lines.pop() ?? "";
    return lines;
  }
}

export function parseInfoLine(line: string): EngineInfo | null {
  if (!line.startsWith("info ")) {
    return null;
  }

  const tokens = line.trim().split(/\s+/);
  const depthIndex = tokens.indexOf("depth");
  const scoreIndex = tokens.indexOf("score");
  const nodesIndex = tokens.indexOf("nodes");
  const selectiveDepthIndex = tokens.indexOf("seldepth");
  const qnodesIndex = tokens.indexOf("qnodes");
  const hashHitsIndex = tokens.indexOf("hashhits");
  const tablebaseHitsIndex = tokens.indexOf("tbhits");
  const pvIndex = tokens.indexOf("pv");

  const scoreType = tokens[scoreIndex + 1];
  const depth = Number(tokens[depthIndex + 1]);
  const scoreValue = Number(tokens[scoreIndex + 2]);
  const nodes = Number(tokens[nodesIndex + 1]);

  if (
    depthIndex < 0 ||
    scoreIndex < 0 ||
    nodesIndex < 0 ||
    (scoreType !== "cp" && scoreType !== "mate") ||
    !Number.isFinite(depth) ||
    !Number.isFinite(scoreValue) ||
    !Number.isFinite(nodes)
  ) {
    return null;
  }

  const result: EngineInfo = {
    depth,
    score: { type: scoreType, value: scoreValue },
    nodes,
    pv: pvIndex >= 0 ? tokens.slice(pvIndex + 1) : [],
  };
  const optionalNumber = (index: number): number | undefined => {
    if (index < 0) {
      return undefined;
    }
    const value = Number(tokens[index + 1]);
    return Number.isFinite(value) ? value : undefined;
  };
  result.selectiveDepth = optionalNumber(selectiveDepthIndex);
  result.quiescenceNodes = optionalNumber(qnodesIndex);
  result.hashHits = optionalNumber(hashHitsIndex);
  result.tablebaseHits = optionalNumber(tablebaseHitsIndex);
  return result;
}

async function findEnginePath(): Promise<string> {
  const configured = process.env.CATFISH_ENGINE_PATH;
  const candidates = [
    configured,
    path.resolve("build/release/catfish_uci"),
    path.resolve("build/debug/catfish_uci"),
    path.resolve("build/make/catfish_uci"),
  ].filter((candidate): candidate is string => Boolean(candidate));

  for (const candidate of candidates) {
    try {
      await access(candidate);
      return candidate;
    } catch {
      // Try the next predictable build location.
    }
  }

  throw new Error(
    "Catfish UCI executable not found. Run `npm run build:engine` or set CATFISH_ENGINE_PATH.",
  );
}

export class UciEngine {
  private process: ChildProcessWithoutNullStreams | null = null;
  private readonly options: EngineOptions;
  private readonly waiters = new Set<LineWaiter>();
  private readonly lineBuffer = new LineBuffer();
  private latestInfo: EngineInfo | null = null;
  private latestSource: EngineInfo["source"] = "search";
  private latestOpening: string | undefined;
  private serial: Promise<void> = Promise.resolve();
  private fatalError: Error | null = null;
  private pendingSearches = 0;

  constructor(options: EngineOptions = {}) {
    this.options = options;
  }

  async start(): Promise<void> {
    if (this.process && !this.fatalError) {
      return;
    }

    const command = this.options.command ?? (await findEnginePath());
    const child = spawn(command, this.options.args ?? [], {
      cwd: process.cwd(),
      env: process.env,
      shell: false,
      stdio: ["pipe", "pipe", "pipe"],
    });

    this.process = child;
    this.fatalError = null;
    child.stdout.setEncoding("utf8");
    child.stderr.setEncoding("utf8");
    child.stdout.on("data", (chunk: string) => {
      for (const line of this.lineBuffer.push(chunk)) {
        this.handleLine(line);
      }
    });
    child.stderr.on("data", (chunk: string) => {
      const diagnostic = chunk.trim();
      if (diagnostic) {
        console.error(`[catfish] ${diagnostic}`);
      }
    });
    child.once("error", (error) => this.fail(error));
    child.once("exit", (code, signal) => {
      if (!this.fatalError) {
        this.fail(
          new Error(
            `Catfish exited unexpectedly (${signal ? `signal ${signal}` : `code ${code ?? "unknown"}`}).`,
          ),
        );
      }
    });

    const uciReady = this.waitForLine((line) => line === "uciok");
    this.write("uci");
    await uciReady;
    const hashSizeMb =
      this.options.hashSizeMb ??
      boundedInteger(process.env.CATFISH_HASH_MB, 32, 1, 1024);
    this.write(`setoption name Hash value ${hashSizeMb}`);
    const ownBook = process.env.CATFISH_OWN_BOOK;
    if (ownBook === "true" || ownBook === "false") {
      this.write(`setoption name OwnBook value ${ownBook}`);
    }
    if (process.env.CATFISH_BOOK_PATH) {
      this.write(`setoption name BookFile value ${process.env.CATFISH_BOOK_PATH}`);
    }
    if (process.env.CATFISH_SYZYGY_PATH) {
      this.write(
        `setoption name SyzygyPath value ${process.env.CATFISH_SYZYGY_PATH}`,
      );
    }
    const ready = this.waitForLine((line) => line === "readyok");
    this.write("isready");
    await ready;
  }

  async search(fen: string, depth: number): Promise<SearchResponse> {
    const maximumPending =
      this.options.maxPendingSearches ??
      boundedInteger(process.env.CATFISH_MAX_PENDING_SEARCHES, 8, 1, 128);
    if (this.pendingSearches >= maximumPending) {
      throw new Error("Catfish is busy. Try again shortly.");
    }
    this.pendingSearches += 1;

    const run = async () => {
      await this.start();
      this.latestInfo = null;
      this.latestSource = "search";
      this.latestOpening = undefined;
      this.write(`position fen ${fen}`);
      const bestMoveLine = this.waitForLine((line) => line.startsWith("bestmove "));
      this.write(`go depth ${depth}`);
      const line = await bestMoveLine;
      const move = line.split(/\s+/)[1] ?? "0000";

      if (move !== "0000" && !UCI_MOVE_PATTERN.test(move)) {
        throw new Error(`Catfish returned an invalid move: ${move}`);
      }

      return {
        bestMove: move === "0000" ? null : move,
        info:
          this.latestInfo ??
          ({
            depth,
            score: { type: "cp", value: 0 },
            nodes: 0,
            pv: [],
          } satisfies EngineInfo),
        fen,
      };
    };

    const task = this.serial.then(run, run);
    this.serial = task.then(
      () => undefined,
      () => undefined,
    );
    return task.finally(() => {
      this.pendingSearches -= 1;
    });
  }

  isReady(): boolean {
    return Boolean(this.process && !this.fatalError);
  }

  getFailure(): string | undefined {
    return this.fatalError?.message;
  }

  async close(): Promise<void> {
    const child = this.process;
    if (!child) {
      return;
    }
    this.process = null;
    this.fatalError = new Error("Catfish engine stopped.");
    if (!child.killed) {
      child.stdin.write("quit\n");
    }
    await new Promise<void>((resolve) => {
      const timer = setTimeout(() => {
        child.kill("SIGTERM");
        resolve();
      }, 500);
      child.once("exit", () => {
        clearTimeout(timer);
        resolve();
      });
    });
  }

  private handleLine(line: string): void {
    if (line.startsWith("info string book ")) {
      this.latestSource = "book";
      this.latestOpening = line.slice("info string book ".length);
    } else if (line.startsWith("info string syzygy ")) {
      this.latestSource = "tablebase";
    }
    const info = parseInfoLine(line);
    if (info) {
      this.latestInfo = {
        ...info,
        source: this.latestSource,
        opening: this.latestOpening,
      };
    }

    for (const waiter of this.waiters) {
      if (waiter.predicate(line)) {
        clearTimeout(waiter.timer);
        this.waiters.delete(waiter);
        waiter.resolve(line);
      }
    }
  }

  private waitForLine(predicate: (line: string) => boolean): Promise<string> {
    const timeoutMs = this.options.timeoutMs ?? 15_000;
    return new Promise((resolve, reject) => {
      const waiter: LineWaiter = {
        predicate,
        resolve,
        reject,
        timer: setTimeout(() => {
          this.waiters.delete(waiter);
          reject(new Error(`Catfish did not respond within ${timeoutMs}ms.`));
        }, timeoutMs),
      };
      this.waiters.add(waiter);
    });
  }

  private write(command: string): void {
    if (!this.process || this.fatalError) {
      throw this.fatalError ?? new Error("Catfish is not running.");
    }
    this.process.stdin.write(`${command}\n`);
  }

  private fail(error: Error): void {
    this.fatalError = error;
    this.process = null;
    for (const waiter of this.waiters) {
      clearTimeout(waiter.timer);
      waiter.reject(error);
    }
    this.waiters.clear();
  }
}
