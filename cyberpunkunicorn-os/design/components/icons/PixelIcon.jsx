import React from 'react';
import { ICONS } from './iconData.js';
import { drawGrid, useTick } from './pixel.js';

export function PixelIcon({ name = 'frequency', cell = 3, animate = true, frame, tick = 0, background, style, title }) {
  const icon = ICONS[name];
  const ref = React.useRef(null);
  const t = useTick(animate, tick);
  const w = 32 * cell, h = 24 * cell;
  React.useEffect(() => {
    const c = ref.current; if (!c || !icon) return;
    const ctx = c.getContext('2d');
    ctx.clearRect(0, 0, w, h);
    if (background) { ctx.fillStyle = background; ctx.fillRect(0, 0, w, h); }
    drawGrid(ctx, icon.grid, icon.mode, t, cell, 0, 0, frame === undefined ? {} : { frame });
  }, [t, icon, cell, frame, background, w, h]);
  if (!icon) return null;
  return React.createElement('canvas', { ref, width: w, height: h, role: 'img', 'aria-label': title || icon.name, style: { width: w, height: h, imageRendering: 'pixelated', display: 'block', ...style } });
}
