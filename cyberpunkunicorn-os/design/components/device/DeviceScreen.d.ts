/** Cheap Yellow Display frame around a 320x240 screen, integer-upscaled. */
export interface DeviceScreenProps {
  /** Content laid out at 320x240 device pixels. */
  children?: React.ReactNode;
  /** Integer upscale. Default 2. */
  scale?: number;
  /** Draw the yellow PCB bezel. Default true. */
  bezel?: boolean;
  /** Board name strip under the screen. Default true. */
  label?: boolean;
  style?: React.CSSProperties;
}
export declare function DeviceScreen(props: DeviceScreenProps): JSX.Element;
