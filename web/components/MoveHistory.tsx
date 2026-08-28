import type { Move } from "chess.js";
import { useEffect, useRef } from "react";

export function MoveHistory({ history }: { history: Move[] }) {
  const listRef = useRef<HTMLOListElement>(null);
  const rows: Array<{ number: number; white?: Move; black?: Move }> = [];
  for (const move of history) {
    const number = Number(move.before.split(/\s+/)[5]);
    const existing = rows.find((row) => row.number === number);
    const row = existing ?? { number };
    if (!existing) rows.push(row);
    if (move.color === "w") row.white = move;
    else row.black = move;
  }

  useEffect(() => {
    const scroller = listRef.current?.closest(".move-list");
    if (scroller instanceof HTMLElement) {
      scroller.scrollTop = scroller.scrollHeight;
    }
  }, [history.length]);

  if (rows.length === 0) {
    return (
      <p className="empty-state">
        The board is set. Make the first move to begin.
      </p>
    );
  }

  return (
    <ol
      ref={listRef}
      className="move-rows"
      aria-label="Move history"
      aria-live="polite"
    >
      {rows.map((row) => (
        <li key={row.number}>
          <span>{row.number}.</span>
          <strong>{row.white?.san ?? "…"}</strong>
          <strong>{row.black?.san ?? ""}</strong>
        </li>
      ))}
    </ol>
  );
}
