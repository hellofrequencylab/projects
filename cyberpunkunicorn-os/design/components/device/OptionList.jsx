import React from 'react';
import { usePhase, hueCss, cyberHue, to565, css } from '../icons/pixel.js';

// cyberItemColor(), lifted toward white in the deep-blue band (200-290 deg) so
// every row stays readable on black (pure 0,0,255 is ~2:1).
function itemColor(h) {
  const d = ((h % 360) + 360) % 360, [r, g, b] = cyberHue(d);
  const k = d >= 200 && d <= 290 ? 0.45 : 0;
  return css(to565([r + (255 - r) * k | 0, g + (255 - g) * k | 0, b + (255 - b) * k | 0]));
}
import { RainbowText } from '../rainbow/RainbowText.jsx';

// loopOptions(MENU_TYPE_SUBMENU) with the rainbow skin: each option its own neon
// colour (cyberItemColor), the selected one inverted in a rainbow-framed box.
export function OptionList({ title, options = [], selected = 0, onSelect, onHover, animate = true, top = 46, maxRows = 7, background = '#000' }) {
  const phase = usePhase(animate);
  const step = Math.max(30, Math.floor(360 / Math.max(1, options.length)));
  const first = Math.max(0, Math.min(selected - Math.floor(maxRows / 2), options.length - maxRows));
  const shown = options.slice(first, first + maxRows);
  const stops = Array.from({ length: 13 }, (_, i) => `${hueCss(phase + i * 30)} ${i * 30}deg`).join(',');
  return React.createElement('div', { style: { position: 'absolute', left: 20, right: 20, top: title ? top - 18 : top, fontFamily: "'Press Start 2P', monospace" } },
    title && React.createElement('div', { style: { display: 'flex', justifyContent: 'center', marginBottom: 6 } }, React.createElement(RainbowText, { size: 1, phase }, title)),
    React.createElement('div', { style: { position: 'relative', padding: 1, borderRadius: 4 } },
      React.createElement('div', { style: { position: 'absolute', inset: 0, borderRadius: 4, padding: 1, background: `conic-gradient(from 90deg, ${stops})`, WebkitMask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)', mask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)' } }),
      React.createElement('div', { style: { position: 'relative', background, borderRadius: 3, padding: '4px 0' } },
        shown.map((o, j) => {
          const i = first + j, on = i === selected, label = typeof o === 'string' ? o : o.label, c = itemColor(300 + i * step);
          return React.createElement('button', { key: i, onClick: () => onSelect && onSelect(i), onMouseEnter: () => onHover && onHover(i), style: { all: 'unset', cursor: 'pointer', display: 'flex', justifyContent: 'space-between', width: '100%', boxSizing: 'border-box', height: 16, alignItems: 'center', padding: '0 8px', fontSize: 6, color: on ? '#000' : c, background: on ? c : 'transparent' } },
            React.createElement('span', null, (on ? '> ' : '  ') + label),
            typeof o === 'object' && o.value != null && React.createElement('span', null, o.value));
        }))),
    options.length > maxRows && React.createElement('div', { style: { textAlign: 'center', fontSize: 6, color: '#7b7b7b', marginTop: 4 } }, `${selected + 1}/${options.length}`));
}
