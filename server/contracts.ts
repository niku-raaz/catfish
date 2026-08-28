import { z } from "zod";

export const searchRequestSchema = z
  .object({
    fen: z.string().trim().min(15).max(120),
    depth: z.number().int().min(1).max(5),
  })
  .strict();

export type SearchRequest = z.infer<typeof searchRequestSchema>;

export type EngineInfo = {
  depth: number;
  selectiveDepth?: number;
  score: {
    type: "cp" | "mate";
    value: number;
  };
  nodes: number;
  quiescenceNodes?: number;
  hashHits?: number;
  tablebaseHits?: number;
  pv: string[];
  source?: "search" | "book" | "tablebase";
  opening?: string;
};

export type SearchResponse = {
  bestMove: string | null;
  info: EngineInfo;
  fen: string;
};

export type HealthResponse = {
  status: "ready" | "starting" | "failed";
  engine: "Catfish";
  detail?: string;
};
