import {
  Chess,
  DEFAULT_POSITION,
  type Color,
  type Move,
  type PieceSymbol,
  type Square,
} from "chess.js";
import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import type { EngineInfo, HealthResponse } from "../../server/contracts";
import { getEngineHealth, searchEngine } from "../api/engineClient";
import { describeGame } from "../chess/gameStatus";

export type PlayerChoice = "white" | "black" | "random";
export type PromotionPiece = Extract<PieceSymbol, "q" | "r" | "b" | "n">;

type Snapshot = {
  fen: string;
  history: Move[];
  pgn: string;
  turn: Color;
  inCheck: boolean;
  gameOver: boolean;
};

export type PendingPromotion = {
  from: Square;
  to: Square;
  color: Color;
};

function takeSnapshot(game: Chess): Snapshot {
  if (game.isCheckmate()) {
    game.setHeader("Result", game.turn() === "b" ? "1-0" : "0-1");
  } else if (game.isDraw()) {
    game.setHeader("Result", "1/2-1/2");
  }
  return {
    fen: game.fen(),
    history: game.history({ verbose: true }),
    pgn: game.pgn(),
    turn: game.turn(),
    inCheck: game.isCheck(),
    gameOver: game.isGameOver(),
  };
}

function asSquare(square: string): Square {
  return square as Square;
}

function makeGame(fen = DEFAULT_POSITION): Chess {
  const game = new Chess(fen);
  game.setHeader("Event", "Catfish local game");
  game.setHeader("White", "You");
  game.setHeader("Black", "Catfish");
  return game;
}

export function useChessGame() {
  const gameRef = useRef(makeGame());
  const depthRef = useRef(3);
  const requestIdRef = useRef(0);
  const [snapshot, setSnapshot] = useState(() => takeSnapshot(gameRef.current));
  const [playerColor, setPlayerColor] = useState<Color>("w");
  const [orientation, setOrientation] = useState<"white" | "black">("white");
  const [depth, setDepthState] = useState(3);
  const [selectedSquare, setSelectedSquare] = useState<Square | null>(null);
  const [legalSquares, setLegalSquares] = useState<Square[]>([]);
  const [pendingPromotion, setPendingPromotion] =
    useState<PendingPromotion | null>(null);
  const [engineStatus, setEngineStatus] = useState<
    "starting" | "ready" | "thinking" | "failed"
  >("starting");
  const [health, setHealth] = useState<HealthResponse | null>(null);
  const [engineInfo, setEngineInfo] = useState<EngineInfo | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [resigned, setResigned] = useState(false);

  const sync = useCallback(() => {
    setSnapshot(takeSnapshot(gameRef.current));
    setSelectedSquare(null);
    setLegalSquares([]);
  }, []);

  const checkHealth = useCallback(async () => {
    setEngineStatus("starting");
    try {
      const response = await getEngineHealth();
      setHealth(response);
      setEngineStatus(response.status === "ready" ? "ready" : response.status);
      if (response.status === "failed") {
        setError(response.detail ?? "The Catfish engine could not start.");
      }
    } catch (cause) {
      setEngineStatus("failed");
      setHealth({
        status: "failed",
        engine: "Catfish",
        detail: cause instanceof Error ? cause.message : "Engine bridge unavailable.",
      });
    }
  }, []);

  useEffect(() => {
    void checkHealth();
  }, [checkHealth]);

  const requestEngineMove = useCallback(
    async (fen: string, engineColor: Color) => {
      const requestId = ++requestIdRef.current;
      setEngineStatus("thinking");
      setError(null);
      try {
        const response = await searchEngine({ fen, depth: depthRef.current });
        if (requestId !== requestIdRef.current || gameRef.current.fen() !== fen) {
          return;
        }

        const info =
          engineColor === "b" && response.info.score.type === "cp"
            ? {
                ...response.info,
                score: {
                  ...response.info.score,
                  value: -response.info.score.value,
                },
              }
            : engineColor === "b" && response.info.score.type === "mate"
              ? {
                  ...response.info,
                  score: {
                    ...response.info.score,
                    value: -response.info.score.value,
                  },
                }
              : response.info;
        setEngineInfo(info);

        if (response.bestMove) {
          const move = gameRef.current.move({
            from: response.bestMove.slice(0, 2),
            to: response.bestMove.slice(2, 4),
            promotion: response.bestMove[4],
          });
          if (!move) {
            throw new Error(`Catfish returned an illegal move: ${response.bestMove}`);
          }
        } else if (!gameRef.current.isGameOver()) {
          throw new Error("Catfish returned no move for a playable position.");
        }

        sync();
        setEngineStatus("ready");
      } catch (cause) {
        if (requestId !== requestIdRef.current) {
          return;
        }
        setEngineStatus("failed");
        setError(
          cause instanceof Error ? cause.message : "Catfish could not complete its move.",
        );
      }
    },
    [sync],
  );

  const finishPlayerMove = useCallback(
    (from: Square, to: Square, promotion?: PromotionPiece): boolean => {
      if (
        engineStatus === "thinking" ||
        resigned ||
        gameRef.current.isGameOver() ||
        gameRef.current.turn() !== playerColor
      ) {
        return false;
      }

      const candidates = gameRef.current
        .moves({ square: from, verbose: true })
        .filter((move) => move.to === to);
      if (candidates.length === 0) {
        return false;
      }
      if (candidates.some((move) => move.isPromotion()) && !promotion) {
        setPendingPromotion({ from, to, color: playerColor });
        return false;
      }

      try {
        gameRef.current.move({ from, to, promotion });
      } catch {
        return false;
      }
      sync();

      if (!gameRef.current.isGameOver()) {
        const fen = gameRef.current.fen();
        void requestEngineMove(fen, gameRef.current.turn());
      }
      return true;
    },
    [engineStatus, playerColor, requestEngineMove, resigned, sync],
  );

  const choosePromotion = useCallback(
    (piece: PromotionPiece) => {
      const pending = pendingPromotion;
      setPendingPromotion(null);
      if (pending) {
        finishPlayerMove(pending.from, pending.to, piece);
      }
    },
    [finishPlayerMove, pendingPromotion],
  );

  const selectSquare = useCallback(
    (squareText: string) => {
      const square = asSquare(squareText);
      if (
        engineStatus === "thinking" ||
        resigned ||
        gameRef.current.isGameOver() ||
        gameRef.current.turn() !== playerColor
      ) {
        return;
      }

      if (selectedSquare) {
        const moved = finishPlayerMove(selectedSquare, square);
        if (moved || pendingPromotion) {
          return;
        }
      }

      const piece = gameRef.current.get(square);
      if (piece?.color === playerColor) {
        const moves = gameRef.current.moves({ square, verbose: true });
        setSelectedSquare(square);
        setLegalSquares([...new Set(moves.map((move) => move.to))]);
      } else {
        setSelectedSquare(null);
        setLegalSquares([]);
      }
    },
    [
      engineStatus,
      finishPlayerMove,
      pendingPromotion,
      playerColor,
      resigned,
      selectedSquare,
    ],
  );

  const startGame = useCallback(
    (choice: PlayerChoice, selectedDepth: number) => {
      requestIdRef.current += 1;
      const color: Color =
        choice === "random"
          ? Math.random() < 0.5
            ? "w"
            : "b"
          : choice === "white"
            ? "w"
            : "b";
      const game = makeGame();
      if (color === "b") {
        game.setHeader("White", "Catfish");
        game.setHeader("Black", "You");
      }
      gameRef.current = game;
      depthRef.current = selectedDepth;
      setDepthState(selectedDepth);
      setPlayerColor(color);
      setOrientation(color === "w" ? "white" : "black");
      setEngineInfo(null);
      setError(null);
      setResigned(false);
      setPendingPromotion(null);
      setEngineStatus(health?.status === "ready" ? "ready" : "starting");
      sync();
      if (color === "b") {
        void requestEngineMove(game.fen(), "w");
      }
    },
    [health?.status, requestEngineMove, sync],
  );

  const loadFen = useCallback(
    (fen: string) => {
      const game = makeGame(fen.trim());
      requestIdRef.current += 1;
      gameRef.current = game;
      setEngineInfo(null);
      setError(null);
      setResigned(false);
      setPendingPromotion(null);
      sync();
      if (!game.isGameOver() && game.turn() !== playerColor) {
        void requestEngineMove(game.fen(), game.turn());
      }
    },
    [playerColor, requestEngineMove, sync],
  );

  const undoTurn = useCallback(() => {
    if (engineStatus === "thinking" || snapshot.history.length === 0) {
      return;
    }
    requestIdRef.current += 1;
    setResigned(false);
    gameRef.current.undo();
    if (
      gameRef.current.history().length > 0 &&
      gameRef.current.turn() !== playerColor
    ) {
      gameRef.current.undo();
    }
    setEngineInfo(null);
    setError(null);
    setEngineStatus("ready");
    sync();
  }, [engineStatus, playerColor, snapshot.history.length, sync]);

  const resign = useCallback(() => {
    if (!snapshot.gameOver && !resigned) {
      requestIdRef.current += 1;
      gameRef.current.setHeader("Result", playerColor === "w" ? "0-1" : "1-0");
      setResigned(true);
      setEngineStatus("ready");
      sync();
    }
  }, [playerColor, resigned, snapshot.gameOver, sync]);

  const setDepth = useCallback((value: number) => {
    depthRef.current = value;
    setDepthState(value);
  }, []);

  const outcome = useMemo(
    () =>
      describeGame(
        gameRef.current,
        playerColor,
        engineStatus === "thinking",
        resigned,
      ),
    [engineStatus, playerColor, resigned, snapshot],
  );

  const lastMove = snapshot.history.at(-1) ?? null;
  const checkSquare =
    snapshot.inCheck
      ? (gameRef.current.findPiece({ type: "k", color: snapshot.turn })[0] ?? null)
      : null;
  const canMove =
    !outcome.terminal &&
    engineStatus !== "thinking" &&
    snapshot.turn === playerColor;

  return {
    snapshot,
    playerColor,
    orientation,
    depth,
    selectedSquare,
    legalSquares,
    pendingPromotion,
    engineStatus,
    engineInfo,
    health,
    error,
    outcome,
    lastMove,
    checkSquare,
    canMove,
    startGame,
    finishPlayerMove,
    choosePromotion,
    selectSquare,
    loadFen,
    undoTurn,
    resign,
    setDepth,
    retryEngine: checkHealth,
    flipBoard: () =>
      setOrientation((current) => (current === "white" ? "black" : "white")),
    dismissError: () => setError(null),
    cancelPromotion: () => setPendingPromotion(null),
  };
}
