import { useState } from "react";
import { CatfishLogo } from "./components/CatfishLogo";
import { Dialogs } from "./components/DialogHost";
import { EvaluationBar, formatEvaluation } from "./components/EvaluationBar";
import { GameBoard } from "./components/GameBoard";
import { MoveHistory } from "./components/MoveHistory";
import { useChessGame } from "./hooks/useChessGame";

function sideName(color: "w" | "b") {
  return color === "w" ? "White" : "Black";
}

export function App() {
  const game = useChessGame();
  const [newGameOpen, setNewGameOpen] = useState(false);
  const [positionOpen, setPositionOpen] = useState(false);
  const playerAtTop =
    (game.orientation === "white" && game.playerColor === "b") ||
    (game.orientation === "black" && game.playerColor === "w");
  const topPlayer = playerAtTop ? "You" : "Catfish";
  const bottomPlayer = playerAtTop ? "Catfish" : "You";
  const engineColor = game.playerColor === "w" ? "b" : "w";
  const engineReady = game.engineStatus === "ready";

  return (
    <main className="app-shell">
      <header className="topbar">
        <a className="brand" href="/" aria-label="Catfish Chess home">
          <span className="brand-mark" aria-hidden="true">
            <CatfishLogo className="brand-logo" />
          </span>
          <span>
            <strong>Catfish</strong>
            <small>C++ chess engine</small>
          </span>
        </a>
        <div className="top-actions">
          <span
            className={`engine-state ${game.engineStatus}`}
            role="status"
            aria-live="polite"
          >
            <i aria-hidden="true" />
            {game.engineStatus === "thinking"
              ? "Engine thinking"
              : engineReady
                ? "Engine ready"
                : game.engineStatus === "starting"
                  ? "Connecting"
                  : "Engine offline"}
          </span>
          <button
            type="button"
            className="quiet-button"
            onClick={() => setNewGameOpen(true)}
          >
            New game
          </button>
        </div>
      </header>

      {game.error && (
        <div className="error-banner" role="alert">
          <span>
            <strong>Engine error</strong>
            {game.error}
          </span>
          <button type="button" className="secondary" onClick={() => void game.retryEngine()}>
            Retry
          </button>
          <button
            type="button"
            className="icon-button"
            aria-label="Dismiss error"
            onClick={game.dismissError}
          >
            ×
          </button>
        </div>
      )}

      <section className="game-layout" aria-label="Chess game">
        <div className="board-column">
          <PlayerRow
            name={topPlayer}
            color={topPlayer === "You" ? game.playerColor : engineColor}
            engine={topPlayer === "Catfish"}
            depth={game.depth}
          />
          <div className="board-stage">
            <EvaluationBar
              info={game.engineInfo}
              orientation={game.orientation}
            />
            <GameBoard {...game} />
          </div>
          <PlayerRow
            name={bottomPlayer}
            color={bottomPlayer === "You" ? game.playerColor : engineColor}
            engine={bottomPlayer === "Catfish"}
            depth={game.depth}
          />
        </div>

        <aside className="game-panel">
          <div className="panel-heading">
            <div>
              <span className="eyebrow">
                {game.outcome.terminal ? "Result" : "Live game"}
              </span>
              <h1>{game.outcome.title}</h1>
              <p>{game.outcome.detail}</p>
            </div>
            <span className="turn-chip">
              {game.outcome.result ?? sideName(game.snapshot.turn)}
            </span>
          </div>

          <div className="analysis-strip">
            <span>White evaluation</span>
            <strong>{formatEvaluation(game.engineInfo)}</strong>
            <span>
              {game.engineInfo
                ? `${game.engineInfo.nodes.toLocaleString()} nodes`
                : "Awaiting search"}
            </span>
          </div>

          <div className="move-list">
            <div className="section-label">
              <span>Moves</span>
              <span>{game.snapshot.history.length} ply</span>
            </div>
            <MoveHistory history={game.snapshot.history} />
          </div>

          <div className="engine-panel">
            <div>
              <span>Search depth</span>
              <strong>
                {game.engineInfo?.depth ?? game.depth}
                {game.engineInfo?.selectiveDepth
                  ? ` / ${game.engineInfo.selectiveDepth}`
                  : ""}
              </strong>
            </div>
            <div>
              <span>Move source</span>
              <strong>
                {game.engineInfo?.source === "book"
                  ? game.engineInfo.opening || "Opening book"
                  : game.engineInfo?.source === "tablebase"
                    ? "Syzygy tablebase"
                    : game.engineInfo?.hashHits
                      ? `${game.engineInfo.hashHits.toLocaleString()} hash hits`
                      : "Search"}
              </strong>
            </div>
            <div>
              <span>Best line</span>
              <strong className="pv">
                {game.engineInfo?.pv.join(" ") || "—"}
              </strong>
            </div>
          </div>

          <div className="game-toolbar" aria-label="Game controls">
            <button
              type="button"
              className="tool-button"
              onClick={game.undoTurn}
              disabled={
                game.engineStatus === "thinking" ||
                game.snapshot.history.length === 0
              }
              title="Undo the previous player and engine moves"
            >
              <span aria-hidden="true">↶</span> Undo turn
            </button>
            <button type="button" className="tool-button" onClick={game.flipBoard}>
              <span aria-hidden="true">⇅</span> Flip board
            </button>
            <button
              type="button"
              className="tool-button"
              onClick={() => setPositionOpen(true)}
            >
              <span aria-hidden="true">⌘</span> Position
            </button>
            <button
              type="button"
              className="tool-button danger"
              onClick={game.resign}
              disabled={game.outcome.terminal}
              title="Resign the current game"
            >
              <span aria-hidden="true">⚑</span> Resign
            </button>
          </div>

          <label className="depth-control">
            <span>
              Strength <strong>Depth {game.depth}</strong>
            </span>
            <input
              type="range"
              min="1"
              max="5"
              step="1"
              value={game.depth}
              disabled={game.engineStatus === "thinking"}
              onChange={(event) => game.setDepth(Number(event.target.value))}
            />
          </label>
        </aside>
      </section>

      <Dialogs
        newGame={{
          open: newGameOpen,
          currentDepth: game.depth,
          onClose: () => setNewGameOpen(false),
          onStart: game.startGame,
        }}
        position={{
          open: positionOpen,
          fen: game.snapshot.fen,
          pgn: game.snapshot.pgn,
          onClose: () => setPositionOpen(false),
          onLoad: game.loadFen,
        }}
        promotion={{
          pending: game.pendingPromotion,
          onChoose: game.choosePromotion,
          onCancel: game.cancelPromotion,
        }}
      />
    </main>
  );
}

function PlayerRow({
  name,
  color,
  engine,
  depth,
}: {
  name: "You" | "Catfish";
  color: "w" | "b";
  engine: boolean;
  depth: number;
}) {
  return (
    <div className="player-row">
      <span className={`avatar ${engine ? "dark" : ""}`} aria-hidden="true">
        {engine ? <CatfishLogo className="avatar-logo" /> : name[0]}
      </span>
      <span>
        <strong>{name}</strong>
        <small>{engine ? `Catfish · depth ${depth}` : sideName(color)}</small>
      </span>
    </div>
  );
}
