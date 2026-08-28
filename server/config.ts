export type ServerConfig = {
  host: string;
  port: number;
};

function parsePort(value: string): number {
  const port = Number(value);
  if (!Number.isInteger(port) || port < 1 || port > 65535) {
    throw new Error(`Invalid server port: ${value}`);
  }
  return port;
}

export function readServerConfig(
  environment: NodeJS.ProcessEnv = process.env,
): ServerConfig {
  return {
    host:
      environment.CATFISH_HOST ??
      (environment.NODE_ENV === "production" ? "0.0.0.0" : "127.0.0.1"),
    port: parsePort(environment.CATFISH_PORT ?? environment.PORT ?? "8787"),
  };
}
