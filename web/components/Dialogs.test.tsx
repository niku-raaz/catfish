import { cleanup, fireEvent, render, screen, waitFor } from "@testing-library/react";
import { afterEach, describe, expect, it, vi } from "vitest";
import { NewGameDialog, PositionDialog, PromotionDialog } from "./Dialogs";

afterEach(() => {
  cleanup();
});

describe("NewGameDialog", () => {
  it("starts a game with the selected side and engine depth", () => {
    const onClose = vi.fn();
    const onStart = vi.fn();
    render(
      <NewGameDialog
        open
        currentDepth={2}
        onClose={onClose}
        onStart={onStart}
      />,
    );

    fireEvent.click(screen.getByRole("radio", { name: "Black" }));
    fireEvent.change(screen.getByRole("slider"), { target: { value: "4" } });
    fireEvent.click(screen.getByRole("button", { name: "Start game" }));

    expect(onStart).toHaveBeenCalledWith("black", 4);
    expect(onClose).toHaveBeenCalledTimes(1);
  });

  it("does not render while closed and closes on Escape", () => {
    const onClose = vi.fn();
    const { rerender } = render(
      <NewGameDialog
        open={false}
        currentDepth={3}
        onClose={onClose}
        onStart={() => undefined}
      />,
    );
    expect(screen.queryByRole("dialog")).not.toBeInTheDocument();

    rerender(
      <NewGameDialog
        open
        currentDepth={3}
        onClose={onClose}
        onStart={() => undefined}
      />,
    );
    fireEvent.keyDown(screen.getByRole("dialog"), { key: "Escape" });
    expect(onClose).toHaveBeenCalledTimes(1);
  });
});

describe("PositionDialog", () => {
  it("loads an edited FEN and closes", () => {
    const onClose = vi.fn();
    const onLoad = vi.fn();
    render(
      <PositionDialog
        open
        fen="initial fen"
        pgn="1. e4"
        onClose={onClose}
        onLoad={onLoad}
      />,
    );

    fireEvent.change(screen.getByLabelText("FEN"), {
      target: { value: "edited fen" },
    });
    fireEvent.click(screen.getByRole("button", { name: "Load position" }));

    expect(onLoad).toHaveBeenCalledWith("edited fen");
    expect(onClose).toHaveBeenCalledTimes(1);
  });

  it("shows a load error and copies FEN and PGN", async () => {
    const writeText = vi.fn().mockResolvedValue(undefined);
    Object.defineProperty(navigator, "clipboard", {
      configurable: true,
      value: { writeText },
    });
    const onLoad = vi.fn(() => {
      throw new Error("Invalid position");
    });
    render(
      <PositionDialog
        open
        fen="fen to copy"
        pgn="pgn to copy"
        onClose={() => undefined}
        onLoad={onLoad}
      />,
    );

    fireEvent.click(screen.getByRole("button", { name: "Copy FEN" }));
    await waitFor(() => expect(writeText).toHaveBeenCalledWith("fen to copy"));
    expect(screen.getByRole("button", { name: "FEN copied" })).toBeVisible();

    fireEvent.click(screen.getByRole("button", { name: "Copy PGN" }));
    await waitFor(() => expect(writeText).toHaveBeenCalledWith("pgn to copy"));
    expect(screen.getByRole("button", { name: "PGN copied" })).toBeVisible();

    fireEvent.click(screen.getByRole("button", { name: "Load position" }));
    expect(screen.getByText("Invalid position")).toBeVisible();
  });
});

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
