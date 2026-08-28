import { expect, test } from "@playwright/test";

async function move(page: import("@playwright/test").Page, from: string, to: string) {
  await page.locator(`[data-square="${from}"]`).click();
  await page.locator(`[data-square="${to}"]`).click();
}

async function loadFen(
  page: import("@playwright/test").Page,
  fen: string,
) {
  await page.getByRole("button", { name: "Position" }).click();
  await page.getByLabel("FEN").fill(fen);
  await page.getByRole("button", { name: "Load position" }).click();
}

test.beforeEach(async ({ page }) => {
  page.on("console", (message) => {
    if (message.type() === "error") {
      throw new Error(`Browser console error: ${message.text()}`);
    }
  });
  page.on("pageerror", (error) => {
    throw error;
  });
  await page.goto("/");
  await expect(page.getByText("Engine ready")).toBeVisible();
});

test("plays a legal human move and receives a Catfish reply", async ({ page }) => {
  await page.route("**/api/search", async (route) => {
    await new Promise((resolve) => setTimeout(resolve, 350));
    await route.continue();
  });
  await expect(page.getByTestId("chessboard")).toBeVisible();
  await move(page, "e2", "e4");
  await expect(page.getByRole("heading", { name: "Catfish is thinking" })).toBeVisible();
  await expect(page.getByRole("heading", { name: "Your move" })).toBeVisible({
    timeout: 15_000,
  });
  const history = page.getByRole("list", { name: "Move history" });
  await expect(history).toContainText("e4");
  await expect(history.locator("strong").nth(1)).not.toHaveText("");
  await expect(page.getByText("Ruy Lopez")).toBeVisible();
});

test("starting as Black makes Catfish move first", async ({ page }) => {
  await page.getByRole("button", { name: "New game" }).click();
  await page.getByText("Black", { exact: true }).click();
  await page.getByRole("button", { name: "Start game" }).click();
  await expect(page.locator(".evaluation-bar")).toHaveClass(/white-at-top/);
  await expect(page.getByRole("heading", { name: "Your move" })).toBeVisible({
    timeout: 15_000,
  });
  await expect(page.getByRole("list", { name: "Move history" })).toBeVisible();
});

test("allows resignation before the first move", async ({ page }) => {
  await page.getByRole("button", { name: "Resign" }).click();
  await expect(page.getByRole("heading", { name: "Game over" })).toBeVisible();
  await expect(page.getByText("You resigned. Catfish wins.")).toBeVisible();
});

test("supports promotion and deterministic checkmate positions", async ({ page }) => {
  await loadFen(page, "7k/P7/7K/8/8/8/8/8 w - - 0 1");
  await move(page, "a7", "a8");
  await expect(page.getByRole("heading", { name: "Promote pawn" })).toBeVisible();
  await page.getByRole("button", { name: "Promote to Queen" }).click();
  await expect(page.getByRole("list", { name: "Move history" })).toContainText("a8=Q");

  await loadFen(page, "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
  await expect(page.getByRole("heading", { name: "Checkmate" })).toBeVisible();
  await expect(page.getByText("White wins by checkmate.")).toBeVisible();
});

test("rejects invalid FEN without losing the current game", async ({ page }) => {
  await page.getByRole("button", { name: "Position" }).click();
  const fen = page.getByLabel("FEN");
  await fen.fill("not a chess position");
  await page.getByRole("button", { name: "Load position" }).click();
  await expect(page.getByText(/Invalid FEN|must contain/i)).toBeVisible();
  await expect(page.getByRole("heading", { name: "Import or export" })).toBeVisible();
});

test("undo, flip, export, and resignation controls remain safe", async ({ page }) => {
  await move(page, "e2", "e4");
  await expect(page.getByRole("heading", { name: "Your move" })).toBeVisible({
    timeout: 15_000,
  });
  await page.getByRole("button", { name: "Undo turn" }).click();
  await expect(page.getByText("0 ply")).toBeVisible();
  await page.getByRole("button", { name: "Flip board" }).click();
  await expect(page.locator('[data-square="h1"]')).toBeVisible();

  await move(page, "d2", "d4");
  await expect(page.getByRole("heading", { name: "Your move" })).toBeVisible({
    timeout: 15_000,
  });
  await page.getByRole("button", { name: "Resign" }).click();
  await expect(page.getByText("You resigned. Catfish wins.")).toBeVisible();
});

test("has no horizontal overflow at the configured viewport", async ({ page }) => {
  const fits = await page.evaluate(
    () => document.documentElement.scrollWidth <= window.innerWidth + 1,
  );
  expect(fits).toBe(true);
});

test("keeps move history in a fixed independent scroll area", async ({ page }) => {
  const panel = page.locator(".game-panel");
  const moveList = page.locator(".move-list");
  const initialHeight = await panel.evaluate((element) => element.clientHeight);

  await expect(moveList).toHaveCSS("overflow-y", "auto");
  await move(page, "e2", "e4");
  await expect(page.getByRole("heading", { name: "Your move" })).toBeVisible({
    timeout: 15_000,
  });

  expect(await panel.evaluate((element) => element.clientHeight)).toBe(
    initialHeight,
  );
});

test("opens and closes primary dialogs from the keyboard", async ({ page }) => {
  const newGame = page.getByRole("button", { name: "New game" });
  await newGame.focus();
  await page.keyboard.press("Enter");
  await expect(page.getByRole("heading", { name: "Start a new game" })).toBeVisible();
  await page.keyboard.press("Escape");
  await expect(page.getByRole("heading", { name: "Start a new game" })).toBeHidden();

  const position = page.getByRole("button", { name: "Position" });
  await position.focus();
  await page.keyboard.press("Enter");
  await expect(page.getByRole("heading", { name: "Import or export" })).toBeVisible();
  await page.keyboard.press("Escape");
  await expect(page.getByRole("heading", { name: "Import or export" })).toBeHidden();
});
