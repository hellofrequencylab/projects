import React from 'react';
import { cyberHue, css, to565 } from '../icons/pixel.js';

// cyberRainbowFill() progress bar with the boot screen's cyan outline.
export function RainbowBar({ value = 0.5, width = 288, height = 10, phase = 0, animate = false, label, showPercent = true, style }) {
  const [p, setP] = React.useState(phase);
  React.useEffect(() => {
    if (!animate) return;
    let raf; const loop = () => { setP((Math.floor(performance.now() / 40) * 10) % 360); raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop); return () => cancelAnimationFrame(raf);
  }, [animate]);
  const ph = animate ? p : phase;
  const pct = Math.round(value * 100);
  const inner = width - 2, done = Math.round(Math.min(1, value) * inner);
  const stops = [];
  for (let x = 0; x <= done; x += 6) stops.push(`${css(to565(cyberHue(x * 2 - ph)))} ${x}px`);
  const over = pct >= 100;
  return React.createElement('div', { style: { width, fontFamily: 'var(--font-display)', fontSize: 6, ...style } },
    React.createElement('div', { style: { width, height, boxSizing: 'border-box', border: '1px solid var(--cu-cyan)' } },
      React.createElement('div', { style: { width: done, height: height - 2, background: `linear-gradient(90deg, ${stops.join(',') || 'transparent'})` } })),
    (label || showPercent) && React.createElement('div', { style: { display: 'flex', justifyContent: 'space-between', marginTop: 5, color: 'var(--cu-white)', lineHeight: '8px' } },
      React.createElement('span', null, label || ''),
      showPercent && React.createElement('span', { style: { color: over ? css(cyberHue(ph * 2)) : 'var(--cu-magenta)' } }, over ? `${pct}% (oops)` : `${pct}%`)));
}
