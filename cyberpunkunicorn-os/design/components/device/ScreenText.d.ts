/** Device text, as tft.drawString(): GFX 6x8 font, absolute device-pixel position. */
export interface ScreenTextProps {
  x?: number;
  y?: number;
  /** GFX text size 1-4 (6x8 px per char per step). */
  size?: 1 | 2 | 3 | 4;
  color?: string;
  /** center = drawCentreString across `width` (default full 320), right = drawRightString ending at x. */
  align?: 'left' | 'center' | 'right';
  width?: number;
  bg?: string;
  children?: React.ReactNode;
  style?: React.CSSProperties;
}
export declare function ScreenText(props: ScreenTextProps): JSX.Element;
