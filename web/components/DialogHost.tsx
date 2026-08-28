import {
  NewGameDialog,
  PositionDialog,
  PromotionDialog,
} from "./Dialogs";

type Props = {
  newGame: React.ComponentProps<typeof NewGameDialog>;
  position: React.ComponentProps<typeof PositionDialog>;
  promotion: React.ComponentProps<typeof PromotionDialog>;
};

export function Dialogs(props: Props) {
  return (
    <>
      <NewGameDialog {...props.newGame} />
      <PositionDialog {...props.position} />
      <PromotionDialog {...props.promotion} />
    </>
  );
}
