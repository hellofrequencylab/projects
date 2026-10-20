import React from 'react';
import { usePhase, hueCss } from '../icons/pixel.js';
import { RainbowText } from '../rainbow/RainbowText.jsx';

const GREY = '#7b7b7b'; // TFT_DARKGREY

export function StatusBar({ phase = 0, label = 'CU5K 0.1.0', clock = '16:35', link = 'USB' }) {
  const line = Array.from({ length: 7 }, (_, i) => hueCss(phase + i * 60)).join(',');
  return React.createElement('div', { style: { position: 'absolute', left: 3, right: 3, top: 3, height: 20, display: 'flex', alignItems: 'center', justifyContent: 'space-between', padding: '0 6px', fontFamily: "'Press Start 2P', monospace", fontSize: 6 } },
    React.createElement(RainbowText, { size: 1, phase }, label),
    React.createElement('span', { style: { display: 'flex', gap: 8 } },
      React.createElement('span', { style: { color: 'var(--cu-cyan)' } }, link),
      React.createElement('span', { style: { color: '#fff' } }, clock)),
    React.createElement('div', { style: { position: 'absolute', left: 0, right: 0, bottom: -1, height: 1, background: `linear-gradient(90deg, ${line})` } }));
}

// drawMainBorderWithTitle() with the rainbow skin: border, status bar, rainbow
// title at y 28, content from y 46 (TOP), dark-grey footer at y 206.
export function AppFrame({ title = '', footer, children, animate = true, phase: fixed, statusLabel, clock, link, background = '#000', backdrop, footerColor }) {
  const live = usePhase(animate);
  const phase = fixed ?? live;
  const stops = Array.from({ length: 13 }, (_, i) => `${hueCss(phase + i * 30)} ${i * 30}deg`).join(',');
  return React.createElement('div', { style: { position: 'absolute', inset: 0, background, overflow: 'hidden' } },
    backdrop,
    React.createElement('div', { style: { position: 'absolute', inset: 1, borderRadius: 5, padding: 1, background: `conic-gradient(from 90deg, ${stops})`, WebkitMask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)', mask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)' } }),
    React.createElement(StatusBar, { phase, label: statusLabel, clock, link }),
    title && React.createElement('div', { style: { position: 'absolute', top: 28, left: 0, right: 0, display: 'flex', justifyContent: 'center' } }, React.createElement(RainbowText, { size: 2, phase }, title)),
    children,
    footer && React.createElement('div', { style: { position: 'absolute', top: 206, left: 0, right: 0, textAlign: 'center', fontFamily: "'Press Start 2P', monospace", fontSize: 6, lineHeight: '8px', color: footerColor || GREY, whiteSpace: 'pre' } }, footer));
}
