import React from 'react';
import { cyberHue, css, to565 } from '../icons/pixel.js';

const hex = c => css(to565(c));
// cyberRainbowText(): each letter a different hue (phase + i*25).
export function RainbowText({ children = '', phase = 0, size = 2, animate = false, as = 'span', style }) {
  const [p, setP] = React.useState(phase);
  React.useEffect(() => {
    if (!animate) return;
    let raf; const loop = () => { setP(Math.floor(performance.now() / 25) % 360); raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop); return () => cancelAnimationFrame(raf);
  }, [animate]);
  const textOf = c => c == null || c === false ? '' : typeof c === 'string' || typeof c === 'number' ? String(c) : Array.isArray(c) ? c.map(textOf).join('') : c.props ? textOf(c.props.children) : '';
  const text = textOf(children);
  const fs = 6 * size;
  return React.createElement(as, { style: { fontFamily: 'var(--font-display)', fontSize: fs, lineHeight: `${8 * size}px`, letterSpacing: 0, whiteSpace: 'pre', ...style } },
    [...text].map((ch, i) => React.createElement('span', { key: i, style: { color: hex(cyberHue((animate ? p : phase) + i * 25)) } }, ch)));
}
