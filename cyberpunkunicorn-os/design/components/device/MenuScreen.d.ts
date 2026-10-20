import type { CyberIconName } from '../icons/PixelIcon';
/**
 * Main menu screen with the rainbow skin: status bar, framed big icon, rainbow label.
 * Render inside DeviceScreen (320x240).
 * @startingPoint section="Device" subtitle="Main menu with animated icon" viewport="700x520"
 */
export interface MenuScreenProps {
  app: CyberIconName;
  /** Position dots: current index and total. */
  index?: number;
  count?: number;
  onPrev?: () => void;
  onNext?: () => void;
  onSelect?: () => void;
  /** Left status-bar text. Default "CU5K 0.1.0". */
  status?: string;
  clock?: string;
  animate?: boolean;
}
export declare function MenuScreen(props: MenuScreenProps): JSX.Element;
