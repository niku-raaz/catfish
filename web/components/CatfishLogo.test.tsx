import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import { CatfishLogo } from "./CatfishLogo";

describe("CatfishLogo", () => {
  it("provides an accessible brand name when used independently", () => {
    render(<CatfishLogo />);
    const logo = screen.getByRole("img", { name: "Catfish chess engine" });

    expect(logo).toBeVisible();
    expect(logo).toHaveAttribute("src", "/catfish-logo.png");
  });
});
