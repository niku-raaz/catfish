import { fireEvent, render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";
import { PromotionDialog } from "./Dialogs";

describe("PromotionDialog", () => {
  it("offers all four promotion pieces and reports the choice", () => {
    const onChoose = vi.fn();
    render(
      <PromotionDialog
        pending={{ from: "a7", to: "a8", color: "w" }}
        onChoose={onChoose}
        onCancel={() => undefined}
      />,
    );
    expect(screen.getByRole("button", { name: "Promote to Queen" })).toBeVisible();
    expect(screen.getByRole("button", { name: "Promote to Rook" })).toBeVisible();
    expect(screen.getByRole("button", { name: "Promote to Bishop" })).toBeVisible();
    fireEvent.click(screen.getByRole("button", { name: "Promote to Knight" }));
    expect(onChoose).toHaveBeenCalledWith("n");
  });
});
