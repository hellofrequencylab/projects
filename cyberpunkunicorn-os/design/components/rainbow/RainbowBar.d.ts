/** Rainbow progress bar with cyan outline and percent, as the boot loader / cyberRainbowFill(). */
export interface RainbowBarProps {
  /** 0-1. Values over 1 print "137% (oops)" like the boot loader. */
  value: number;
  width?: number;
  height?: number;
  phase?: number;
  animate?: boolean;
  /** Left caption, e.g. "Reticulating rainbows...". */
  label?: string;
  showPercent?: boolean;
  style?: React.CSSProperties;
}
export declare function RainbowBar(props: RainbowBarProps): JSX.Element;
