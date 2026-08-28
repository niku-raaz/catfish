import type {
  HealthResponse,
  SearchRequest,
  SearchResponse,
} from "../../server/contracts";

async function requestJson<T>(
  input: RequestInfo | URL,
  init?: RequestInit,
): Promise<T> {
  const response = await fetch(input, init);
  const body = (await response.json()) as T & { error?: string };
  if (!response.ok) {
    throw new Error(body.error ?? `Request failed with status ${response.status}.`);
  }
  return body;
}

export function getEngineHealth(): Promise<HealthResponse> {
  return requestJson<HealthResponse>("/api/health");
}

export function searchEngine(request: SearchRequest): Promise<SearchResponse> {
  return requestJson<SearchResponse>("/api/search", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(request),
  });
}
