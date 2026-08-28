import type { EngineInfo } from "../../server/contracts";

export function formatEvaluation(info: EngineInfo | null): string {
  if (!info) return "0.00";
  if (info.score.type === "mate") {
    return `${info.score.value >= 0 ? "+" : "−"}M${Math.abs(info.score.value)}`;
  }
  const pawns = info.score.value / 100;
  return `${pawns >= 0 ? "+" : "−"}${Math.abs(pawns).toFixed(2)}`;
}

export function EvaluationBar({
  info,
  orientation,
}: {
  info: EngineInfo | null;
  orientation: "white" | "black";
}) {
  const whitePercent =
    info?.score.type === "mate"
      ? info.score.value > 0
        ? 96
        : 4
      : Math.max(4, Math.min(96, 50 + (info?.score.value ?? 0) / 12));

  return (
    <div
      className={`evaluation-bar white-at-${orientation === "white" ? "bottom" : "top"}`}
      aria-label={`Evaluation from White's perspective; White is at the ${orientation === "white" ? "bottom" : "top"}`}
    >
      <span style={{ height: `${whitePercent}%` }} />
      <strong>{formatEvaluation(info)}</strong>
    </div>
  );
}
