type CatfishLogoProps = {
  className?: string;
};

export function CatfishLogo({ className }: CatfishLogoProps) {
  return (
    <img
      className={className}
      src="/catfish-logo.png"
      alt="Catfish chess engine"
      draggable="false"
    />
  );
}
