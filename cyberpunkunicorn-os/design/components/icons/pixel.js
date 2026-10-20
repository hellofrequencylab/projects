// Shared pixel engine: the firmware's colour maths, ported 1:1.
import React from 'react';
import { PALETTE } from './iconData.js';

// cyberHue(): full-saturation hue (deg) -> [r,g,b]. Same sector maths as rainbow.cpp.
export function cyberHue(hue) {
  let h = hue % 360; if (h < 0) h += 360;
  const s = Math.floor(h / 60), f = Math.floor((h % 60) * 255 / 60);
  switch (s) {
    case 0: return [255, f, 0];
    case 1: return [255 - f, 255, 0];
    case 2: return [0, 255, f];
    case 3: return [0, 255 - f, 255];
    case 4: return [f, 0, 255];
    default: return [255, 0, 255 - f];
  }
}
// Quantise to RGB565 and back, so the web shows what the panel shows.
export function to565([r, g, b]) { return [(r & 0xf8) | (r >> 5), (g & 0xfc) | (g >> 6), (b & 0xf8) | (b >> 5)]; }
export const css = ([r, g, b]) => `rgb(${r},${g},${b})`;

export function modeHue(mode, x, y, w = 32, h = 24) {
  if (mode === 'row') return y * 18;
  if (mode === 'col') return x * 12;
  if (mode === 'diag') return (x + y) * 10;
  return Math.atan2(y - (h - 1) / 2, x - (w - 1) / 2) * 180 / Math.PI + 180;
}

const LUT = Object.fromEntries(PALETTE.map(p => [p.k, p.v]));

// Colour of one cell at tick t, or null for nothing drawn.
export function cellColor(k, x, y, mode, t, opts = {}) {
  const v = LUT[k];
  if (v === null || v === undefined) return null;
  const frame = opts.frame ?? (Math.floor(t / 6) % 2);
  const hueOf = () => cyberHue(modeHue(mode, x, y) + t * 12);
  if (v === 'rainbow') return hueOf();
  if (v === 'frameA') return frame === 0 ? hueOf() : null;
  if (v === 'frameB') return frame === 1 ? hueOf() : null;
  if (v === 'gridA') return frame === 0 ? [0, 160, 255] : [12, 0, 28];
  if (v === 'gridB') return frame === 1 ? [0, 160, 255] : [12, 0, 28];
  if (v === 'sparkle') {
    const p = (x * 3 + y * 5 + Math.floor(t / 4)) % 6;
    return p === 0 ? [255, 255, 255] : p === 1 ? [0, 255, 255] : p === 2 ? [255, 90, 200] : null;
  }
  return v;
}

export function drawGrid(ctx, grid, mode, t, cell, ox = 0, oy = 0, opts = {}) {
  for (let y = 0; y < grid.length; y++) {
    const row = grid[y];
    for (let x = 0; x < row.length; x++) {
      const c = cellColor(row[x], x, y, mode, t, opts);
      if (!c) continue;
      ctx.fillStyle = css(opts.raw ? c : to565(c));
      ctx.fillRect(ox + x * cell, oy + y * cell, cell, cell);
    }
  }
}

// cyberPhase(): hue offset drifting 1 degree per 25 ms.
export function usePhase(running = true, start = 0) {
  const [p, setP] = React.useState(start);
  React.useEffect(() => {
    if (!running) { setP(start); return; }
    let raf, last = 0;
    const loop = now => { if (now - last >= 40) { last = now; setP(Math.floor(now / 25) % 360); } raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [running, start]);
  return p;
}
export const hueCss = h => css(to565(cyberHue(h)));

// 25 fps tick counter (FRAME_MS = 40). Pass false to freeze at `start`.
export function useTick(running = true, start = 0) {
  const [t, setT] = React.useState(start);
  React.useEffect(() => {
    if (!running) { setT(start); return; }
    let raf, last = performance.now(), acc = 0;
    const loop = now => { acc += now - last; last = now; if (acc >= 40) { setT(v => v + Math.floor(acc / 40)); acc %= 40; } raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [running, start]);
  return t;
}
