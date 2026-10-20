/**
 * The unicorn boot screen: synthwave sky, striped sun, racing grid, galloping
 * unicorn with rainbow trail, glitch title and the 137% loader. 320x240.
 * Finale: at 46% the unicorn trembles through the loader stall; at 60% it banks
 * left toward the horizon and rockets over the sun into the distance, spraying a
 * rainbow jet at the viewer; from ~82% the spray floods the whole screen and the
 * tagline appears over it, ready to cut to the menu.
 * @startingPoint section="Device" subtitle="Unicorn boot screen (animated)" viewport="700x500"
 */
export interface BootSceneProps {
  /** BOOT_TITLE, max 26 chars, ASCII. Default "CYBERUNICORN 5000". */
  title?: string;
  /** BOOT_TAGLINE shown once the bar passes 100%. */
  tagline?: string;
  /** Scene length in ms. Default 10000 with the finale, 7000 without. */
  duration?: number;
  /** Play the blast-off finale. false = the classic loader: the unicorn just
   *  gallops and bobs while the bar overshoots to 137%. Default true. */
  finale?: boolean;
  /** Restart when finished. Default true. */
  loop?: boolean;
  /** Integer upscale for the web. Default 1. */
  scale?: number;
  onDone?: () => void;
  style?: React.CSSProperties;
}
export declare function BootScene(props: BootSceneProps): JSX.Element;
