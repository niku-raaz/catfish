import type { Move } from "chess.js";
import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import { MoveHistory } from "./MoveHistory";

function fakeMove(index: number): Move {
  const fullMove = Math.floor(index / 2) + 1;
  return {
    before: `8/8/8/8/8/8/8/8 ${index % 2 === 0 ? "w" : "b"} - - 0 ${fullMove}`,
    color: index % 2 === 0 ? "w" : "b",
    san: index % 2 === 0 ? "Nf3" : "Nf6",
  } as Move;
}

describe("MoveHistory", () => {
  it("keeps the latest move visible inside its own scroll container", () => {
    const initial = Array.from({ length: 20 }, (_, index) => fakeMove(index));
    const { rerender } = render(
      <div className="move-list" data-testid="move-scroller">
        <MoveHistory history={initial} />
      </div>,
    );
    const scroller = screen.getByTestId("move-scroller");
    Object.defineProperty(scroller, "scrollHeight", {
      configurable: true,
      value: 720,
    });

    rerender(
      <div className="move-list" data-testid="move-scroller">
        <MoveHistory history={[...initial, fakeMove(20)]} />
      </div>,
    );

    expect(scroller.scrollTop).toBe(720);
    expect(screen.getByRole("list", { name: "Move history" })).toHaveAttribute(
      "aria-live",
      "polite",
    );
  });
});
