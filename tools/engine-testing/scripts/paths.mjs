import { access } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const workspaceRoot = path.resolve(
  path.dirname(fileURLToPath(import.meta.url)),
  "../../..",
);

export function engineTestingRoot() {
  return path.resolve(workspaceRoot, "tools/engine-testing");
}

export function resultsDir() {
  return path.join(engineTestingRoot(), "results");
}

async function isRunnableBinary(candidate) {
  try {
    await access(candidate);
  } catch {
    return false;
  }
  // Skip foreign-OS artifacts left in build/ (Mach-O / ELF) that access() still sees.
  if (process.platform === "win32") {
    const { open } = await import("node:fs/promises");
    const handle = await open(candidate, "r");
    try {
      const buffer = Buffer.alloc(4);
      await handle.read(buffer, 0, 4, 0);
      const isPe = buffer[0] === 0x4d && buffer[1] === 0x5a; // MZ
      return isPe;
    } finally {
      await handle.close();
    }
  }
  return true;
}

export async function resolveCatfishUci() {
  const configured =
    process.env.CATFISH_UCI ?? process.env.CATFISH_ENGINE_PATH ?? "";
  const candidates = [
    configured,
    path.resolve(workspaceRoot, "build/release/catfish_uci.exe"),
    path.resolve(workspaceRoot, "build/release/catfish_uci"),
    path.resolve(workspaceRoot, "build/debug/catfish_uci.exe"),
    path.resolve(workspaceRoot, "build/debug/catfish_uci"),
    path.resolve(workspaceRoot, "build/make/catfish_uci.exe"),
    path.resolve(workspaceRoot, "build/make/catfish_uci"),
  ].filter(Boolean);

  for (const candidate of candidates) {
    if (await isRunnableBinary(candidate)) {
      return candidate;
    }
  }

  throw new Error(
    "catfish_uci not found. Build with `npm run build:engine:release` " +
      "or set CATFISH_UCI / CATFISH_ENGINE_PATH.",
  );
}

export async function resolveBinary(envName, fallbackName) {
  const configured = process.env[envName];
  if (configured) {
    await access(configured);
    return configured;
  }
  throw new Error(
    `${fallbackName} path required. Set ${envName} to an absolute binary path.`,
  );
}

export function quoteArg(value) {
  if (/[\s"]/u.test(value)) {
    return `"${value.replaceAll('"', '\\"')}"`;
  }
  return value;
}
