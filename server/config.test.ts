// @vitest-environment node
import { describe, expect, it } from "vitest";
import { readServerConfig } from "./config.js";

describe("production server configuration", () => {
  it("honors platform PORT and binds publicly in production", () => {
    expect(readServerConfig({ NODE_ENV: "production", PORT: "10000" })).toEqual({
      host: "0.0.0.0",
      port: 10000,
    });
  });

  it("allows Catfish-specific overrides and rejects invalid ports", () => {
    expect(
      readServerConfig({
        CATFISH_HOST: "127.0.0.2",
        CATFISH_PORT: "9000",
        PORT: "10000",
      }),
    ).toEqual({ host: "127.0.0.2", port: 9000 });
    expect(() => readServerConfig({ PORT: "not-a-port" })).toThrow(
      "Invalid server port",
    );
  });
});
