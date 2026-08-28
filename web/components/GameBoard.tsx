import { Chess } from "chess.js";
import { Chessboard } from "react-chessboard";
import type { ReturnTypeOfGameHook } from "./types";

type Props = Pick<
  ReturnTypeOfGameHook,
  | "snapshot"
  | "orientation"
  | "selectedSquare"
  | "legalSquares"
  | "lastMove"
  | "checkSquare"
  | "canMove"
  | "finishPlayerMove"
  | "selectSquare"
>;

export function GameBoard({
  snapshot,
  orientation,
  selectedSquare,
  legalSquares,
  lastMove,
  checkSquare,
  canMove,
  finishPlayerMove,
  selectSquare,
}: Props) {
  const position = new Chess(snapshot.fen);
  const squareStyles: Record<string, React.CSSProperties> = {};

  if (lastMove) {
    squareStyles[lastMove.from] = { backgroundColor: "rgba(224, 236, 113, 0.32)" };
    squareStyles[lastMove.to] = { backgroundColor: "rgba(224, 236, 113, 0.42)" };
  }
  if (selectedSquare) {
    squareStyles[selectedSquare] = {
      boxShadow: "inset 0 0 0 4px rgba(182, 227, 110, 0.85)",
    };
  }
  for (const square of legalSquares) {
    squareStyles[square] = position.get(square)
      ? {
          ...squareStyles[square],
          boxShadow: "inset 0 0 0 5px rgba(182, 227, 110, 0.62)",
        }
      : {
          ...squareStyles[square],
          backgroundImage:
            "radial-gradient(circle, rgba(39, 55, 38, .55) 0 15%, transparent 17%)",
        };
  }
  if (checkSquare) {
    squareStyles[checkSquare] = {
      ...squareStyles[checkSquare],
      background:
        "radial-gradient(circle, rgba(216, 88, 72, .85), rgba(216, 88, 72, .24) 62%, transparent 70%)",
    };
  }

  return (
    <div
      className="board-frame"
      aria-label={`Chessboard, ${orientation} orientation`}
      data-testid="chessboard"
    >
      <Chessboard
        options={{
          id: "catfish-board",
          position: snapshot.fen,
          boardOrientation: orientation,
          allowDragging: canMove,
          canDragPiece: ({ square }) => {
            if (!canMove || !square) return false;
            return position.get(square as never)?.color === snapshot.turn;
          },
          onPieceDrop: ({ sourceSquare, targetSquare }) =>
            Boolean(
              targetSquare &&
                finishPlayerMove(
                  sourceSquare as never,
                  targetSquare as never,
                ),
            ),
          onSquareClick: ({ square }) => selectSquare(square),
          squareStyles,
          showAnimations: true,
          animationDurationInMs: 170,
          boardStyle: { borderRadius: "3px" },
          darkSquareStyle: { backgroundColor: "#55715d" },
          lightSquareStyle: { backgroundColor: "#dce4d6" },
          darkSquareNotationStyle: { color: "rgba(235, 242, 230, .72)" },
          lightSquareNotationStyle: { color: "rgba(45, 69, 51, .7)" },
        }}
      />
    </div>
  );
}
