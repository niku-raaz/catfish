import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import { EvaluationBar, formatEvaluation } from "./EvaluationBar";

describe("formatEvaluation", () => {
  it("formats white-perspective centipawn and mate scores", () => {
    expect(formatEvaluation(null)).toBe("0.00");
    expect(
      formatEvaluation({
        depth: 3,
        score: { type: "cp", value: 34 },
        nodes: 20,
        pv: [],
      }),
    ).toBe("+0.34");
    expect(
      formatEvaluation({
        depth: 4,
        score: { type: "mate", value: -2 },
        nodes: 99,
        pv: [],
      }),
    ).toBe("−M2");
  });

  it("places White's segment beside White after the board flips", () => {
    const { rerender } = render(
      <EvaluationBar info={null} orientation="white" />,
    );
    const bar = screen.getByLabelText(/White is at the bottom/);
    expect(bar).toHaveClass("white-at-bottom");

    rerender(<EvaluationBar info={null} orientation="black" />);
    expect(screen.getByLabelText(/White is at the top/)).toHaveClass(
      "white-at-top",
    );
  });
});
