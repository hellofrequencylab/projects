export interface OptionItem { label: string; value?: string; }
/** On-device pick list (Bruce loopOptions submenu) with per-option neon colours and a rainbow box. */
export interface OptionListProps {
  /** Small rainbow caption above the box. */
  title?: string;
  options: Array<string | OptionItem>;
  selected?: number;
  onSelect?: (index: number) => void;
  onHover?: (index: number) => void;
  animate?: boolean;
  /** Top of the list in device px. Default 46. */
  top?: number;
  /** Rows visible before scrolling. Default 7. */
  maxRows?: number;
  /** Fill inside the rainbow box. Default black; use a translucent dark over backdrops. */
  background?: string;
}
export declare function OptionList(props: OptionListProps): JSX.Element;
