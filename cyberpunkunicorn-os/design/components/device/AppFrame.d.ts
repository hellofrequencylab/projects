/**
 * Full-screen app frame with the rainbow skin (drawMainBorderWithTitle):
 * rainbow border, status bar, rainbow title at y 28, content from y 46, footer at y 206.
 * Render inside DeviceScreen; children are absolutely positioned in device px.
 */
export interface AppFrameProps {
  /** Screen title, uppercase, max ~24 chars at size 2. */
  title?: string;
  /** Key hints, e.g. "SEL toggle    ESC exit". */
  footer?: string;
  children?: React.ReactNode;
  /** Drift the rainbow (cyberPhase). Default true. */
  animate?: boolean;
  /** Freeze the hue phase at this value. */
  phase?: number;
  statusLabel?: string;
  clock?: string;
  /** Connection badge in the status bar. Default "USB". */
  link?: string;
  /** Screen fill under everything. Default black. */
  background?: string;
  /** Full-screen layer drawn under the border and title (e.g. a synthwave scene). */
  backdrop?: React.ReactNode;
  /** Footer colour; default TFT_DARKGREY. Use a light colour over a backdrop. */
  footerColor?: string;
}
export declare function AppFrame(props: AppFrameProps): JSX.Element;

/** The status bar alone (rainbow OS label, link badge, clock, rainbow rule). */
export interface StatusBarProps { phase?: number; label?: string; clock?: string; link?: string; }
export declare function StatusBar(props: StatusBarProps): JSX.Element;
