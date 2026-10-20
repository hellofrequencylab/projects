import React from 'react';
import { cyberHue, css, to565 } from '../icons/pixel.js';

// cyberRainbowFrame(): an outline whose colour walks the hue wheel around the perimeter.
export function RainbowFrame({ children, thickness = 1, phase = 0, animate = false, radius = 0, padding = 8, style }) {
  const [p, setP] = React.useState(phase);
  React.useEffect(() => {
    if (!animate) return;
    let raf; const loop = () => { setP(Math.floor(performance.now() / 25) % 360); raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop); return () => cancelAnimationFrame(raf);
  }, [animate]);
  const ph = animate ? p : phase;
  const stops = Array.from({ length: 13 }, (_, i) => css(to565(cyberHue(ph + i * 30))) + ` ${i * 30}deg`).join(',');
  return React.createElement('div', { style: { position: 'relative', padding: thickness, borderRadius: radius, background: `conic-gradient(from 90deg, ${stops})`, ...style } },
    React.createElement('div', { style: { background: 'var(--surface-screen)', borderRadius: Math.max(0, radius - thickness), padding, height: '100%', boxSizing: 'border-box' } }, children));
}
