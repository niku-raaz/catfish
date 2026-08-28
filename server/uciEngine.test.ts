// @vitest-environment node
import { afterEach, describe, expect, it } from "vitest";
import { fileURLToPath } from "node:url";
import { searchRequestSchema } from "./contracts.js";
import {
  engineExecutableName,
  LineBuffer,
  parseInfoLine,
  UciEngine,
} from "./uciEngine.js";

const engines: UciEngine[] = [];
const fakeUciFixturePath = fileURLToPath(
  new URL("./test/fake-uci.mjs", import.meta.url),
);

afterEach(async () => {
  await Promise.all(engines.map((engine) => engine.close()));
  engines.length = 0;
});

describe("UCI stream parsing", () => {
  it("uses the platform-specific engine executable name", () => {
    expect(engineExecutableName("win32")).toBe("catfish_uci.exe");
    expect(engineExecutableName("linux")).toBe("catfish_uci");
  });

  it("preserves partial lines and emits complete lines", () => {
    const buffer = new LineBuffer();
    expect(buffer.push("ready")).toEqual([]);
    expect(buffer.push("ok\ninfo one\npartial")).toEqual([
      "readyok",
      "info one",
    ]);
    expect(buffer.push(" line\n")).toEqual(["partial line"]);
  });

  it("parses centipawn and mate search information", () => {
    expect(
      parseInfoLine("info depth 4 score cp -18 nodes 901 pv e7e5 g1f3"),
    ).toEqual({
      depth: 4,
      selectiveDepth: undefined,
      score: { type: "cp", value: -18 },
      nodes: 901,
      quiescenceNodes: undefined,
      hashHits: undefined,
      tablebaseHits: undefined,
      pv: ["e7e5", "g1f3"],
    });
    expect(parseInfoLine("info depth 2 score mate 1 nodes 8 pv h7h8q")).toEqual({
      depth: 2,
      selectiveDepth: undefined,
      score: { type: "mate", value: 1 },
      nodes: 8,
      quiescenceNodes: undefined,
      hashHits: undefined,
      tablebaseHits: undefined,
      pv: ["h7h8q"],
    });
    expect(
      parseInfoLine(
        "info depth 5 seldepth 9 score cp 18 nodes 13960 qnodes 11694 tbhits 0 hashhits 616 pv b1c3",
      ),
    ).toMatchObject({
      depth: 5,
      selectiveDepth: 9,
      quiescenceNodes: 11694,
      tablebaseHits: 0,
      hashHits: 616,
    });
  });
});

describe("UciEngine", () => {
  it("handshakes, combines chunked output, and returns a search", async () => {
    const engine = new UciEngine({
      command: process.execPath,
      args: [fakeUciFixturePath],
      timeoutMs: 1_000,
    });
    engines.push(engine);

    await engine.start();
    const result = await engine.search(
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      3,
    );
    expect(result.bestMove).toBe("e2e4");
    expect(result.info.nodes).toBe(1200);
    expect(result.info.source).toBe("book");
    expect(result.info.opening).toBe("Ruy Lopez");
  });

  it("bounds the number of active and queued searches", async () => {
    const engine = new UciEngine({
      command: process.execPath,
      args: [fakeUciFixturePath],
      timeoutMs: 1_000,
      maxPendingSearches: 1,
    });
    engines.push(engine);
    await engine.start();

    const first = engine.search(
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      3,
    );
    await expect(
      engine.search(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        3,
      ),
    ).rejects.toThrow("Catfish is busy");
    await expect(first).resolves.toMatchObject({ bestMove: "e2e4" });
  });
});

describe("request validation", () => {
  it("bounds depth and rejects unknown fields", () => {
    expect(
      searchRequestSchema.safeParse({
        fen: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        depth: 3,
      }).success,
    ).toBe(true);
    expect(
      searchRequestSchema.safeParse({
        fen: "valid-looking-but-short",
        depth: 9,
        command: "rm",
      }).success,
    ).toBe(false);
  });
});
