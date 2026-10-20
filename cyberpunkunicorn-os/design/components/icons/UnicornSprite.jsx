import React from 'react';
import { UNICORN_FRAMES, UNICORN_PALETTE } from './unicornData.js';
import { cyberHue, to565, css, useTick } from './pixel.js';

export function UnicornSprite({ cell = 3, animate = true, frame, style }) {
  const ref = React.useRef(null);
  const t = useTick(animate);
  const W = 40, H = 29;
  React.useEffect(() => {
    const ctx = ref.current && ref.current.getContext('2d'); if (!ctx) return;
    ctx.clearRect(0, 0, W * cell, H * cell);
    const f = frame ?? Math.floor(t / 4) % 2;
    UNICORN_FRAMES[f].forEach((row, y) => {
      const mane = cyberHue(y * 18 + t * 12);
      [...row].forEach((k, x) => {
        if (k === '.') return;
        ctx.fillStyle = css(to565(k === 'R' ? mane : UNICORN_PALETTE[k]));
        ctx.fillRect(x * cell, y * cell, cell, cell);
      });
    });
  }, [t, cell, frame]);
  return React.createElement('canvas', { ref, width: W * cell, height: H * cell, role: 'img', 'aria-label': 'CyberUnicorn', style: { imageRendering: 'pixelated', display: 'block', ...style } });
}
