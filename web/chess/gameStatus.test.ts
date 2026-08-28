import { Chess } from "chess.js";
import { describe, expect, it } from "vitest";
import { describeGame } from "./gameStatus";

describe("describeGame", () => {
  it("reports the player's turn and check", () => {
    const start = new Chess();
    expect(describeGame(start, "w", false, false).title).toBe("Your move");

    const checked = new Chess("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1");
    expect(describeGame(checked, "w", false, false).title).toContain("check");
  });

  it("distinguishes checkmate, stalemate, and resignation", () => {
    const mate = new Chess("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
    expect(describeGame(mate, "w", false, false)).toMatchObject({
      title: "Checkmate",
      result: "1-0",
      terminal: true,
    });

    const stalemate = new Chess("7k/5Q2/7K/8/8/8/8/8 b - - 0 1");
    expect(describeGame(stalemate, "w", false, false).title).toBe("Stalemate");

    expect(describeGame(new Chess(), "b", false, true).result).toBe("1-0");
  });
});
