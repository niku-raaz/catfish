import type { Chess } from "chess.js";

export type GameOutcome = {
  title: string;
  detail: string;
  result?: "1-0" | "0-1" | "1/2-1/2";
  terminal: boolean;
};

export function describeGame(
  game: Chess,
  playerColor: "w" | "b",
  engineThinking: boolean,
  resigned: boolean,
): GameOutcome {
  if (resigned) {
    return {
      title: "Game over",
      detail: "You resigned. Catfish wins.",
      result: playerColor === "w" ? "0-1" : "1-0",
      terminal: true,
    };
  }
  if (game.isCheckmate()) {
    const whiteWon = game.turn() === "b";
    return {
      title: "Checkmate",
      detail: whiteWon ? "White wins by checkmate." : "Black wins by checkmate.",
      result: whiteWon ? "1-0" : "0-1",
      terminal: true,
    };
  }
  if (game.isStalemate()) {
    return {
      title: "Stalemate",
      detail: "No legal moves. The game is drawn.",
      result: "1/2-1/2",
      terminal: true,
    };
  }
  if (game.isInsufficientMaterial()) {
    return {
      title: "Draw",
      detail: "Draw by insufficient material.",
      result: "1/2-1/2",
      terminal: true,
    };
  }
  if (game.isThreefoldRepetition()) {
    return {
      title: "Draw",
      detail: "Draw by threefold repetition.",
      result: "1/2-1/2",
      terminal: true,
    };
  }
  if (game.isDrawByFiftyMoves()) {
    return {
      title: "Draw",
      detail: "Draw by the fifty-move rule.",
      result: "1/2-1/2",
      terminal: true,
    };
  }
  if (engineThinking) {
    return {
      title: "Catfish is thinking",
      detail: "Your board is locked while the engine searches.",
      terminal: false,
    };
  }
  if (game.turn() === playerColor) {
    return {
      title: game.isCheck() ? "Your king is in check" : "Your move",
      detail: game.isCheck() ? "Find a legal reply." : "Choose a piece to continue.",
      terminal: false,
    };
  }
  return {
    title: "Catfish to move",
    detail: "Waiting for the engine.",
    terminal: false,
  };
}
