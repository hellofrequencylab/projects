import React from 'react';
import { PixelIcon } from '../icons/PixelIcon.jsx';
import { RainbowText } from '../rainbow/RainbowText.jsx';
import { StatusBar } from './AppFrame.jsx';
import { ICONS } from '../icons/iconData.js';
import { cyberHue, css, to565 } from '../icons/pixel.js';

// Main menu with the rainbow skin: one big icon, label, prev/next arrows.
// Layout is an approximation of Bruce 1.16.1's main menu, not a pixel copy.
export function MenuScreen({ app = 'frequency', index, count, onPrev, onNext, onSelect, status = 'CU5K 0.1.0', clock = '16:35', animate = true }) {
  const icon = ICONS[app];
  const [ph, setPh] = React.useState(0);
  React.useEffect(() => {
    if (!animate) return;
    let raf; const loop = () => { setPh(Math.floor(performance.now() / 25) % 360); raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop); return () => cancelAnimationFrame(raf);
  }, [animate]);
  const stops = Array.from({ length: 13 }, (_, i) => css(to565(cyberHue(ph + i * 30))) + ` ${i * 30}deg`).join(',');
  const arrow = (dir, fn) => React.createElement('button', { onClick: fn, 'aria-label': dir < 0 ? 'Previous' : 'Next', style: { all: 'unset', cursor: 'pointer', width: 28, height: 72, display: 'grid', placeItems: 'center' } },
    React.createElement('div', { style: { width: 0, height: 0, borderTop: '9px solid transparent', borderBottom: '9px solid transparent', [dir < 0 ? 'borderRight' : 'borderLeft']: '9px solid var(--cu-magenta)' } }));
  return React.createElement('div', { style: { position: 'absolute', inset: 0, background: '#000', fontFamily: 'var(--font-display)', color: '#fff' } },
    React.createElement(StatusBar, { phase: ph, label: status, clock }),
    React.createElement('div', { style: { position: 'absolute', left: 4, right: 4, top: 24, bottom: 4, padding: 1, background: `conic-gradient(from 90deg, ${stops})`, borderRadius: 5 } },
      React.createElement('div', { style: { width: '100%', height: '100%', background: '#000', borderRadius: 4, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 14 } },
        React.createElement('div', { style: { display: 'flex', alignItems: 'center', gap: 18 } }, arrow(-1, onPrev),
          React.createElement('button', { onClick: onSelect, style: { all: 'unset', cursor: 'pointer' } }, React.createElement(PixelIcon, { name: app, cell: 3, animate })),
          arrow(1, onNext)),
        React.createElement(RainbowText, { size: 2, phase: ph }, icon ? icon.name : app),
        index !== undefined && count ? React.createElement('div', { style: { display: 'flex', gap: 3 } }, Array.from({ length: count }, (_, i) => React.createElement('div', { key: i, style: { width: 3, height: 3, background: i === index ? css(to565(cyberHue(300 + i * 30))) : '#301040' } }))) : null)));
}
