/** The boot-screen unicorn (40x29, two gallop frames, rainbow mane). */
export interface UnicornSpriteProps {
  /** Device pixels per sprite pixel. 3 = boot scene size (UNI_SCALE). */
  cell?: number;
  animate?: boolean;
  /** Pin gallop frame 0 or 1. */
  frame?: 0 | 1;
  style?: React.CSSProperties;
}
export declare function UnicornSprite(props: UnicornSpriteProps): JSX.Element;
