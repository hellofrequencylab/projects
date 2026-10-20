export type CyberIconName =
  | 'frequency' | 'lightshow' | 'sensors' | 'led' | 'light' | 'sound' | 'slideshow' | 'weather'
  | 'hat' | 'gpio' | 'remote' | 'wifi' | 'ble' | 'rf' | 'rfid' | 'ir' | 'nrf24' | 'gps'
  | 'scripts' | 'others' | 'clock' | 'connect' | 'config';

/**
 * Animated 32x24 pixel-art app icon, rendered exactly as the firmware draws it
 * (RGB565, rainbow indices, sparkles, A/B frames).
 * @startingPoint section="Icons" subtitle="Animated pixel-art app icon" viewport="700x320"
 */
export interface PixelIconProps {
  /** Icon id from icons/build_icons.js. */
  name: CyberIconName;
  /** Device pixels per sprite cell. 3 = on-device size (96x72). */
  cell?: number;
  /** Run the 25 fps clock. Default true. */
  animate?: boolean;
  /** Pin the A/B frame (0 or 1) instead of alternating. */
  frame?: 0 | 1;
  /** Starting tick when not animating. */
  tick?: number;
  /** Fill behind the sprite (CSS colour). Default transparent. */
  background?: string;
  title?: string;
  style?: React.CSSProperties;
}
export declare function PixelIcon(props: PixelIconProps): JSX.Element;
