import express, {
  type ErrorRequestHandler,
  type Express,
  type RequestHandler,
} from "express";
import { Chess } from "chess.js";
import { existsSync } from "node:fs";
import path from "node:path";
import {
  searchRequestSchema,
  type HealthResponse,
  type SearchResponse,
} from "./contracts.js";
import type { UciEngine } from "./uciEngine.js";

function positiveEnvironmentInteger(
  name: string,
  fallback: number,
  maximum: number,
): number {
  const value = Number(process.env[name] ?? fallback);
  return Number.isInteger(value) && value > 0 && value <= maximum
    ? value
    : fallback;
}

export function createApp(engine: UciEngine): Express {
  const app = express();
  app.disable("x-powered-by");
  if (process.env.NODE_ENV === "production") {
    app.set("trust proxy", 1);
  }
  app.use((_request, response, next) => {
    response.setHeader("X-Content-Type-Options", "nosniff");
    response.setHeader("X-Frame-Options", "DENY");
    response.setHeader("Referrer-Policy", "strict-origin-when-cross-origin");
    response.setHeader(
      "Permissions-Policy",
      "camera=(), microphone=(), geolocation=()",
    );
    next();
  });
  app.use("/api", (_request, response, next) => {
    response.setHeader("Cache-Control", "no-store");
    next();
  });
  app.use(express.json({ limit: "8kb", strict: true }));

  app.get("/api/health", (_request, response) => {
    const body: HealthResponse = engine.isReady()
      ? { status: "ready", engine: "Catfish" }
      : {
          status: engine.getFailure() ? "failed" : "starting",
          engine: "Catfish",
          detail: engine.getFailure(),
        };
    response.status(body.status === "failed" ? 503 : 200).json(body);
  });

  const rateLimitWindowMs = positiveEnvironmentInteger(
    "CATFISH_RATE_LIMIT_WINDOW_MS",
    60_000,
    3_600_000,
  );
  const rateLimitMaximum = positiveEnvironmentInteger(
    "CATFISH_RATE_LIMIT_MAX",
    60,
    10_000,
  );
  const searchRequests = new Map<string, { count: number; resetAt: number }>();
  const searchRateLimit: RequestHandler = (request, response, next) => {
    const now = Date.now();
    const key = request.ip ?? request.socket.remoteAddress ?? "unknown";
    const current = searchRequests.get(key);
    const bucket =
      !current || current.resetAt <= now
        ? { count: 0, resetAt: now + rateLimitWindowMs }
        : current;
    bucket.count += 1;
    searchRequests.set(key, bucket);
    response.setHeader(
      "RateLimit-Policy",
      `${rateLimitMaximum};w=${Math.ceil(rateLimitWindowMs / 1000)}`,
    );
    response.setHeader(
      "RateLimit-Remaining",
      String(Math.max(0, rateLimitMaximum - bucket.count)),
    );
    if (bucket.count > rateLimitMaximum) {
      response.setHeader(
        "Retry-After",
        String(Math.max(1, Math.ceil((bucket.resetAt - now) / 1000))),
      );
      response.status(429).json({ error: "Too many engine requests. Try again shortly." });
      return;
    }
    if (searchRequests.size > 10_000) {
      for (const [address, entry] of searchRequests) {
        if (entry.resetAt <= now) {
          searchRequests.delete(address);
        }
      }
    }
    next();
  };

  const searchHandler: RequestHandler = async (request, response, next) => {
    try {
      const parsed = searchRequestSchema.safeParse(request.body);
      if (!parsed.success) {
        response.status(400).json({
          error: "Invalid search request.",
          issues: parsed.error.issues.map((issue) => issue.message),
        });
        return;
      }

      try {
        new Chess(parsed.data.fen);
      } catch {
        response.status(400).json({ error: "Invalid FEN position." });
        return;
      }

      const result: SearchResponse = await engine.search(
        parsed.data.fen,
        parsed.data.depth,
      );
      response.json(result);
    } catch (error) {
      next(error);
    }
  };

  app.post("/api/search", searchRateLimit, searchHandler);

  const webRoot = path.resolve("dist-web");
  if (existsSync(webRoot)) {
    app.use(
      "/assets",
      express.static(path.join(webRoot, "assets"), {
        immutable: true,
        maxAge: "1y",
      }),
    );
    app.use(express.static(webRoot, { index: false, maxAge: "1h" }));
    app.use((request, response, next) => {
      if (
        request.method === "GET" &&
        !request.path.startsWith("/api/") &&
        request.accepts("html")
      ) {
        response.setHeader("Cache-Control", "no-cache");
        response.sendFile(path.join(webRoot, "index.html"));
        return;
      }
      next();
    });
  }

  const errorHandler: ErrorRequestHandler = (error, _request, response, _next) => {
    console.error(error);
    response.status(503).json({
      error:
        error instanceof Error
          ? error.message
          : "The Catfish engine is unavailable.",
    });
  };
  app.use(errorHandler);

  return app;
}
