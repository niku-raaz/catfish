import { useEffect, useState } from "react";
import type {
  PendingPromotion,
  PlayerChoice,
  PromotionPiece,
} from "../hooks/useChessGame";

type NewGameProps = {
  open: boolean;
  currentDepth: number;
  onClose: () => void;
  onStart: (choice: PlayerChoice, depth: number) => void;
};

export function NewGameDialog({
  open,
  currentDepth,
  onClose,
  onStart,
}: NewGameProps) {
  const [choice, setChoice] = useState<PlayerChoice>("white");
  const [depth, setDepth] = useState(currentDepth);

  useEffect(() => setDepth(currentDepth), [currentDepth]);
  if (!open) return null;

  return (
    <div className="modal-backdrop" onMouseDown={onClose}>
      <dialog
        className="modal"
        open
        aria-labelledby="new-game-title"
        onMouseDown={(event) => event.stopPropagation()}
        onKeyDown={(event) => event.key === "Escape" && onClose()}
      >
        <span className="eyebrow">Fresh board</span>
        <h2 id="new-game-title">Start a new game</h2>
        <p>Choose your side and how deeply Catfish should calculate.</p>

        <fieldset>
          <legend>Play as</legend>
          <div className="segmented">
            {(["white", "black", "random"] as const).map((value) => (
              <label key={value}>
                <input
                  type="radio"
                  name="color"
                  value={value}
                  checked={choice === value}
                  onChange={() => setChoice(value)}
                  autoFocus={value === "white"}
                />
                <span>{value[0].toUpperCase() + value.slice(1)}</span>
              </label>
            ))}
          </div>
        </fieldset>

        <label className="field">
          <span>
            Engine depth <strong>{depth}</strong>
          </span>
          <input
            type="range"
            min="1"
            max="5"
            step="1"
            value={depth}
            onChange={(event) => setDepth(Number(event.target.value))}
          />
          <small>Higher depth is stronger but takes longer.</small>
        </label>

        <div className="modal-actions">
          <button type="button" className="secondary" onClick={onClose}>
            Cancel
          </button>
          <button
            type="button"
            onClick={() => {
              onStart(choice, depth);
              onClose();
            }}
          >
            Start game
          </button>
        </div>
      </dialog>
    </div>
  );
}

type PositionProps = {
  open: boolean;
  fen: string;
  pgn: string;
  onClose: () => void;
  onLoad: (fen: string) => void;
};

export function PositionDialog({
  open,
  fen,
  pgn,
  onClose,
  onLoad,
}: PositionProps) {
  const [value, setValue] = useState(fen);
  const [localError, setLocalError] = useState<string | null>(null);
  const [copied, setCopied] = useState<"fen" | "pgn" | null>(null);

  useEffect(() => {
    if (open) {
      setValue(fen);
      setLocalError(null);
    }
  }, [fen, open]);
  if (!open) return null;

  async function copy(text: string, kind: "fen" | "pgn") {
    await navigator.clipboard.writeText(text);
    setCopied(kind);
    window.setTimeout(() => setCopied(null), 1200);
  }

  return (
    <div className="modal-backdrop" onMouseDown={onClose}>
      <dialog
        className="modal position-modal"
        open
        aria-labelledby="position-title"
        onMouseDown={(event) => event.stopPropagation()}
        onKeyDown={(event) => event.key === "Escape" && onClose()}
      >
        <span className="eyebrow">Position tools</span>
        <h2 id="position-title">Import or export</h2>
        <label className="field">
          <span>FEN</span>
          <textarea
            value={value}
            rows={3}
            autoFocus
            onChange={(event) => setValue(event.target.value)}
          />
        </label>
        {localError && <p className="field-error">{localError}</p>}
        <div className="copy-row">
          <button type="button" className="secondary" onClick={() => void copy(fen, "fen")}>
            {copied === "fen" ? "FEN copied" : "Copy FEN"}
          </button>
          <button type="button" className="secondary" onClick={() => void copy(pgn, "pgn")}>
            {copied === "pgn" ? "PGN copied" : "Copy PGN"}
          </button>
        </div>
        <div className="modal-actions">
          <button type="button" className="secondary" onClick={onClose}>
            Close
          </button>
          <button
            type="button"
            onClick={() => {
              try {
                onLoad(value);
                onClose();
              } catch (cause) {
                setLocalError(
                  cause instanceof Error ? cause.message : "That FEN is invalid.",
                );
              }
            }}
          >
            Load position
          </button>
        </div>
      </dialog>
    </div>
  );
}

export function PromotionDialog({
  pending,
  onChoose,
  onCancel,
}: {
  pending: PendingPromotion | null;
  onChoose: (piece: PromotionPiece) => void;
  onCancel: () => void;
}) {
  if (!pending) return null;
  const symbols =
    pending.color === "w"
      ? { q: "♕", r: "♖", b: "♗", n: "♘" }
      : { q: "♛", r: "♜", b: "♝", n: "♞" };
  const names = { q: "Queen", r: "Rook", b: "Bishop", n: "Knight" };

  return (
    <div className="modal-backdrop promotion-backdrop" onMouseDown={onCancel}>
      <dialog
        className="modal promotion-modal"
        open
        aria-labelledby="promotion-title"
        onMouseDown={(event) => event.stopPropagation()}
        onKeyDown={(event) => event.key === "Escape" && onCancel()}
      >
        <h2 id="promotion-title">Promote pawn</h2>
        <div className="promotion-pieces">
          {(["q", "r", "b", "n"] as const).map((piece, index) => (
            <button
              type="button"
              key={piece}
              className="piece-choice"
              aria-label={`Promote to ${names[piece]}`}
              autoFocus={index === 0}
              onClick={() => onChoose(piece)}
            >
              {symbols[piece]}
            </button>
          ))}
        </div>
      </dialog>
    </div>
  );
}
