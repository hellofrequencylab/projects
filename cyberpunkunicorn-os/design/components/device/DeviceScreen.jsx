import React from 'react';

// The CYD: 2.8" 320x240 ILI9341 on a yellow PCB. `scale` must be an integer.
export function DeviceScreen({ children, scale = 2, bezel = true, label = true, style }) {
  const screen = React.createElement('div', { style: { width: 320, height: 240, position: 'relative', overflow: 'hidden', background: '#000', transform: `scale(${scale})`, transformOrigin: '0 0' } }, children);
  const sized = React.createElement('div', { style: { width: 320 * scale, height: 240 * scale, imageRendering: 'pixelated' } }, screen);
  if (!bezel) return React.createElement('div', { style }, sized);
  return React.createElement('div', { style: { display: 'inline-flex', flexDirection: 'column', gap: 8, padding: 14, paddingBottom: label ? 10 : 14, background: '#f2c200', borderRadius: 6, boxShadow: 'inset 0 0 0 2px #c99a00', ...style } },
    React.createElement('div', { style: { padding: 6, background: '#111', borderRadius: 2 } }, sized),
    label && React.createElement('div', { style: { display: 'flex', justifyContent: 'space-between', fontFamily: 'var(--font-display)', fontSize: 7, color: '#3a2a00' } },
      React.createElement('span', null, 'ESP32-2432S028'), React.createElement('span', null, 'CU5K')));
}
