import "dotenv/config";
import { createServer } from "node:http";
import { createApp } from "./app.js";
import { readServerConfig } from "./config.js";
import { UciEngine } from "./uciEngine.js";

const { host, port } = readServerConfig();
const engine = new UciEngine();
const app = createApp(engine);
const server = createServer(app);

try {
  await engine.start();
} catch (error) {
  console.error(
    error instanceof Error ? error.message : "Unable to start Catfish.",
  );
}

server.listen(port, host, () => {
  console.log(`Catfish bridge listening at http://${host}:${port}`);
});

let shuttingDown = false;
async function shutdown() {
  if (shuttingDown) {
    return;
  }
  shuttingDown = true;
  await new Promise<void>((resolve) => {
    server.close(() => resolve());
  });
  await engine.close();
}

process.once("SIGINT", () => void shutdown());
process.once("SIGTERM", () => void shutdown());
