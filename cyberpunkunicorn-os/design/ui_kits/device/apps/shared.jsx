// Shared bits for the app screens: D-pad routing and the fx helpers the
// firmware uses (fxWave rainbow wave, synthwave backdrop).
const CU = window.CyberUnicorn5000DesignSystem_f09c19;
const GREY = '#7b7b7b', LGREY = '#c6c6c6';
const cyberHue = h0 => { let h = h0 % 360; if (h < 0) h += 360; const s = Math.floor(h / 60), f = Math.floor((h % 60) * 255 / 60); return [[255, f, 0], [255 - f, 255, 0], [0, 255, f], [0, 255 - f, 255], [f, 0, 255], [255, 0, 255 - f]][s]; };
const q565 = ([r, g, b]) => `rgb(${(r & 0xf8) | (r >> 5)},${(g & 0xfc) | (g >> 6)},${(b & 0xf8) | (b >> 5)})`;
const hue = h => q565(cyberHue(h));

// nav = {k:'prev'|'next'|'sel'|'esc', n}. Calls handler once per press.
// Inactive hooks still record presses, so a screen never replays the key that opened it.
function useNav(nav, handler, active = true) {
  const ref = React.useRef(handler); ref.current = handler;
  const act = React.useRef(active); act.current = active;
  const seen = React.useRef(nav && nav.n);
  React.useEffect(() => { if (nav && nav.n !== seen.current) { seen.current = nav.n; if (act.current) ref.current(nav.k); } }, [nav]);
}

// A pick list screen bound to the D-pad. Returns [index, setIndex].
function useMenu(nav, count, onSel, onEsc, active = true) {
  const [i, setI] = React.useState(0);
  useNav(nav, k => {
    if (k === 'prev') setI(v => (v + count - 1) % count);
    if (k === 'next') setI(v => (v + 1) % count);
    if (k === 'sel') onSel(i);
    if (k === 'esc') onEsc();
  }, active);
  return [i, setI];
}

function useTicker(ms = 40) {
  const [t, setT] = React.useState(0);
  React.useEffect(() => { const id = setInterval(() => setT(v => v + 1), ms); return () => clearInterval(id); }, [ms]);
  return t;
}

// fxWave(): rainbow sine ribbons while waiting for a tag.
function Wave({ x = 12, y = 88, w = 296, h = 110, clear = false }) {
  const ref = React.useRef(null), t = useTicker();
  React.useEffect(() => {
    const c = ref.current.getContext('2d'); if (clear) c.clearRect(0, 0, w, h); else { c.fillStyle = '#000'; c.fillRect(0, 0, w, h); }
    for (let b = 0; b < 6; b++) for (let i = 0; i < w; i++) {
      const yy = h / 2 + Math.sin((i + t * 4) / 18 + b * 0.5) * (h / 2 - 8) * Math.sin((i + t * 2) / 60) + (b - 2.5) * 3;
      c.fillStyle = hue(b * 60 - t * 8 + i); c.fillRect(i, Math.round(yy), 1, 3);
    }
  }, [t]);
  return <canvas ref={ref} width={w} height={h} style={{ position: 'absolute', left: x, top: y, imageRendering: 'pixelated' }} />;
}

// drawBackdrop() from WeatherMenu.cpp: sky gradient, striped sun, magenta grid.
function Synthwave({ sun = true, horizon = 146, sunX = 268, sunR = 30 }) {
  const ref = React.useRef(null);
  React.useEffect(() => {
    const c = ref.current.getContext('2d'), W = 320, H = 240, hy = horizon;
    for (let y = 0; y < hy; y++) { const k = y * 255 / hy; c.fillStyle = `rgb(${10 + k * 90 / 255 | 0},${k * 10 / 255 | 0},${40 + k * 60 / 255 | 0})`; c.fillRect(0, y, W, 1); }
    if (sun) { const cx = sunX, r = sunR; for (let dy = -r; dy < 0; dy++) { const band = -dy; if (band < r * 0.47 && (band % 5) < (5 - (band / (r / 10) | 0))) continue; const half = Math.sqrt(r * r - dy * dy) | 0, k = (dy + r) * 255 / r; c.fillStyle = `rgb(255,${220 - k * 180 / 255 | 0},${60 + k * 120 / 255 | 0})`; c.fillRect(cx - half, hy + dy, 2 * half, 1); } }
    c.fillStyle = '#000'; c.fillRect(0, hy, W, H - hy);
    for (let x = 0; x < W; x++) { c.fillStyle = hue(x * 360 / W); c.fillRect(x, hy, 1, 1); }
    c.fillStyle = hue(300); for (let i = 1, y = hy + 3; y < H; i++, y += 2 + i * 2) c.fillRect(0, y, W, 1);
    c.strokeStyle = hue(300); for (let i = -8; i <= 8; i++) { c.beginPath(); c.moveTo(W / 2 + i * 12 + .5, hy); c.lineTo(W / 2 + i * 60 + .5, H); c.stroke(); }
  }, [sun, horizon, sunX, sunR]);
  return <canvas ref={ref} width={320} height={240} style={{ position: 'absolute', inset: 0 }} />;
}

// Twinkling 4-point sparkles over a screen; density = star count. burst = confetti
// shooting out from a point (for success screens).
function Sparkles({ count = 26, seed = 7, burst, top = 0, bottom = 240 }) {
  const ref = React.useRef(null), t = useTicker(70);
  React.useEffect(() => {
    const c = ref.current.getContext('2d'); c.clearRect(0, 0, 320, 240);
    const C = ['#ffffff', q565([0, 255, 255]), q565([255, 90, 200]), q565([255, 220, 60]), q565([0, 255, 160])];
    const star = (x, y, r, col) => { c.fillStyle = col; c.fillRect(x - r, y, 2 * r + 1, 1); c.fillRect(x, y - r, 1, 2 * r + 1); if (r > 2) { c.fillStyle = '#fff'; c.fillRect(x - 1, y - 1, 3, 3); } };
    for (let i = 0; i < count; i++) {
      const x = (i * 97 + seed * 31) % 316 + 2, y = top + (i * 53 + seed * 17) % (bottom - top), ph = (t + i * 5) % 18;
      const r = ph < 3 ? ph : ph < 6 ? 6 - ph : 0; if (r) star(x, y, r + (i % 3 === 0 ? 1 : 0), C[i % 5]);
    }
    if (burst) for (let i = 0; i < 36; i++) {
      const a = i * 10 * Math.PI / 180, d = ((t * 4 + i * 7) % 90) + 6, x = burst[0] + Math.cos(a) * d | 0, y = burst[1] + Math.sin(a) * d * 0.7 | 0;
      c.fillStyle = hue(i * 40 + t * 12); c.fillRect(x, y, 3, 3); if (i % 4 === 0) star(x, y, 2, '#fff');
    }
  }, [t]);
  return <canvas ref={ref} width={320} height={240} style={{ position: 'absolute', inset: 0, pointerEvents: 'none' }} />;
}
// Dark glass panel with a rainbow hairline, for content over a backdrop.
function Panel({ x, y, w, h, children }) {
  const stops = Array.from({ length: 7 }, (_, i) => hue(300 + i * 60)).join(',');
  const ring = 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)';
  return <div style={{ position: 'absolute', left: x, top: y, width: w, height: h, borderRadius: 4, background: 'rgba(12,0,28,.86)' }}>
    <div style={{ position: 'absolute', inset: 0, padding: 1, borderRadius: 4, background: `linear-gradient(135deg, ${stops})`, WebkitMask: ring, mask: ring }}></div>
    <div style={{ position: 'absolute', inset: 1 }}>{children}</div></div>;
}
// line(): grey label at x 14, coloured value at x 82 (FrequencyMenu.cpp).
function Line({ y, label, value, color = '#fff' }) {
  const { ScreenText } = CU;
  return <><ScreenText x={14} y={y} color={GREY}>{label}</ScreenText><ScreenText x={82} y={y} color={color}>{value}</ScreenText></>;
}

// Bruce's scrolling text area: plain lines from y 46.
function TextArea({ lines, top = 46, color = '#fff' }) {
  const { ScreenText } = CU;
  return <>{lines.map((l, i) => <ScreenText key={i} x={14} y={top + i * 10} color={typeof l === 'object' ? l.c : color}>{typeof l === 'object' ? l.t : l}</ScreenText>)}</>;
}

// App top level: OptionList menu; SEL opens screens[i] (null = stays on menu).
// Screens mount fresh, so their own useNav never sees the key that opened them.
function MenuApp({ nav, onExit, initial = 'menu', title, options, screens, icon }) {
  const { AppFrame, OptionList, PixelIcon } = CU;
  const [screen, setScreen] = React.useState(initial);
  const open = idx => screens[idx] && setScreen(idx);
  const [i, setI] = useMenu(nav, options.length, open, onExit, screen === 'menu');
  if (screen !== 'menu') { const S = screens[screen]; return <S nav={nav} back={() => setScreen('menu')} />; }
  return <AppFrame title={title}>
    {icon && <div style={{ position: 'absolute', left: 0, right: 0, top: 48, display: 'flex', justifyContent: 'center' }}><PixelIcon name={icon} cell={2} /></div>}
    <OptionList options={options} selected={i} onSelect={idx => { setI(idx); open(idx); }} top={icon ? 102 : 52} maxRows={icon ? 6 : 8} />
  </AppFrame>;
}

Object.assign(window, { CU, GREY, LGREY, hue, cyberHue, q565, useNav, useMenu, useTicker, Wave, Synthwave, Sparkles, Panel, Line, TextArea, MenuApp });
