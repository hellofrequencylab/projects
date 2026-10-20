/** Per-letter rainbow text, as cyberRainbowText() in apps/ui/rainbow.cpp. */
export interface RainbowTextProps {
  children: string;
  /** Hue offset in degrees. Default 0. */
  phase?: number;
  /** GFX text size 1-3 (6x8 px per char per step). Default 2. */
  size?: 1 | 2 | 3;
  /** Drift the phase like cyberPhase() (millis/25). */
  animate?: boolean;
  as?: 'span' | 'div' | 'h1' | 'h2' | 'h3' | 'p';
  style?: React.CSSProperties;
}
export declare function RainbowText(props: RainbowTextProps): JSX.Element;
