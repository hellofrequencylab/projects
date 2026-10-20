import React from 'react';

// tft.drawString(): GFX 6x8 font at size 1-4, positioned in device pixels.
export function ScreenText({ x = 0, y = 0, size = 1, color = '#fff', align = 'left', width, bg, children, style }) {
  const fs = 6 * size;
  const pos = align === 'center' ? { left: width ? x : 0, width: width || 320, textAlign: 'center' } : align === 'right' ? { right: 320 - x, textAlign: 'right' } : { left: x };
  return React.createElement('div', { style: { position: 'absolute', top: y, ...pos, fontFamily: "'Press Start 2P', monospace", fontSize: fs, lineHeight: `${8 * size}px`, height: 8 * size, paddingTop: size, boxSizing: 'border-box', color, background: bg, whiteSpace: 'pre', letterSpacing: 0, ...style } }, children);
}
