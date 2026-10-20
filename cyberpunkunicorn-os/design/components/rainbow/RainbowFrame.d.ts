/** Container with a rainbow outline, as cyberRainbowFrame() (borders, submenus). */
export interface RainbowFrameProps {
  children?: React.ReactNode;
  /** Border thickness in px. Default 1. */
  thickness?: number;
  phase?: number;
  /** Drift the hue like cyberPhase(). */
  animate?: boolean;
  /** Corner radius px. Default 0 (the device draws square frames). */
  radius?: number;
  padding?: number;
  style?: React.CSSProperties;
}
export declare function RainbowFrame(props: RainbowFrameProps): JSX.Element;
