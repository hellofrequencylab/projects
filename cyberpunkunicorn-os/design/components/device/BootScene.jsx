import React from 'react';
import { cyberHue, css, to565 } from '../icons/pixel.js';
import { UNICORN_FRAMES, UNICORN_PALETTE } from '../icons/unicornData.js';

// Canvas port of unicornBootScene() in apps/boot/unicorn_boot.cpp, extended with
// the blast-off finale: gallop, tremble, rocket away on a rainbow jet, flood the screen.
const TITLE_H = 34, SCENE_H = 170, HORIZON = 110, UNI = 3, SPARKS = 18;
const LOADING = ['Polishing horn...', 'Reticulating rainbows...', 'Downloading more RAM...', 'Charging glitter capacitors...', 'Bribing the CC1101...', 'Untangling the mane...', 'Installing hooves.dll...', 'Teaching IR to see colour...', 'Negotiating with the nRF24...', 'Overclocking sparkles...', 'Summoning the PN532 spirit...', 'Calibrating vibes...'];
const c565 = (r, g, b) => css(to565([r, g, b]));
const hue = h => css(to565(cyberHue(h)));

function drawText(ctx, s, x, y, size, color, right) {
  ctx.font = `${6 * size}px 'Press Start 2P', monospace`;
  ctx.textBaseline = 'top'; ctx.fillStyle = color;
  const w = s.length * 6 * size; let x0 = right ? x - w : x;
  for (let i = 0; i < s.length; i++) ctx.fillText(s[i], x0 + i * 6 * size, y + size);
}

export function BootScene({ title = 'CYBERUNICORN 5000', tagline = 'All systems fabulous.', duration, loop = true, scale = 1, finale = true, onDone, style }) {
  duration = duration ?? (finale ? 10000 : 7000);
  const ref = React.useRef(null);
  React.useEffect(() => {
    const cv = ref.current, ctx = cv.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    let rng = 0xC0FFEE42 >>> 0;
    const rnd = () => { rng = (Math.imul(rng, 1664525) + 1013904223) >>> 0; return rng >>> 8; };
    let sparks = Array.from({ length: SPARKS }, () => ({ x: 0, y: 0, life: 0 }));
    let t = 0, start = performance.now(), raf, last = 0, done = false;
    const titleCv = document.createElement('canvas'); titleCv.width = 320; titleCv.height = TITLE_H;
    const tctx = titleCv.getContext('2d');

    const sky = t => {
      for (let y = 0; y < HORIZON; y++) { const k = y * 255 / HORIZON | 0; ctx.fillStyle = c565(30 + k / 3 | 0, 0, 70 - k / 6 | 0); ctx.fillRect(0, TITLE_H + y, 320, 1); }
      ctx.fillStyle = '#fff';
      for (let i = 0; i < 24; i++) { const x = (i * 97 + 13) % 320, y = (i * 53 + 7) % (HORIZON - 30); if (((i * 7 + (t / 6 | 0)) % 9) !== 0) ctx.fillRect(x, TITLE_H + y, 1, 1); }
      const cx = 228, cy = HORIZON, r = 46;
      for (let y = cy - r; y < cy; y++) {
        const dy = cy - y, dx = Math.sqrt(r * r - dy * dy) | 0;
        if (dy < 22 && dy % 6 < 2) continue;
        const k = (y - (cy - r)) * 255 / r | 0;
        ctx.fillStyle = c565(255, 220 - (k * 160 / 255 | 0), 60 + (k * 120 / 255 | 0)); ctx.fillRect(cx - dx, TITLE_H + y, 2 * dx, 1);
      }
    };
    const grid = t => {
      const depth = SCENE_H - HORIZON, o = TITLE_H;
      ctx.fillStyle = c565(12, 0, 28); ctx.fillRect(0, o + HORIZON, 320, depth);
      const phase = (t % 24) / 24;
      for (let i = 0; i < 9; i++) { const z = (i + phase) / 9, y = HORIZON + (z * z * depth | 0); ctx.fillStyle = c565(0, 160 + (z * 95 | 0), 255); ctx.fillRect(0, o + y, 320, 1); }
      ctx.strokeStyle = c565(255, 0, 200); ctx.lineWidth = 1;
      ctx.save(); ctx.beginPath(); ctx.rect(0, o + HORIZON, 320, depth); ctx.clip();
      for (let i = -11; i <= 11; i++) { ctx.beginPath(); ctx.moveTo(160 + i * 9 + 0.5, o + HORIZON); ctx.lineTo(160 + i * 64 + 0.5, o + SCENE_H - 1); ctx.stroke(); }
      ctx.restore();
      ctx.fillStyle = c565(255, 120, 255); ctx.fillRect(0, o + HORIZON, 320, 1);
    };
    const trail = (ux, uy, t) => {
      const top = uy + 15 * UNI;
      for (let x = 0; x < ux + 6 * UNI; x++) { const wave = 4 * Math.sin((x + t * 6) / 11) | 0; for (let b = 0; b < 6; b++) { ctx.fillStyle = hue(b * 60 - t * 8); ctx.fillRect(x, TITLE_H + top + b * 4 + wave, 1, 4); } }
    };
    const sparksDraw = (ux, uy, t) => sparks.forEach((k, i) => {
      if (k.life === 0) { k.x = rnd() % (ux + 40 > 0 ? ux + 40 : 1); k.y = uy + 30 + rnd() % 50 - 25; k.life = 6 + rnd() % 10; }
      k.x -= 3; k.life--;
      const a = k.life > 4 ? 2 : 1; ctx.fillStyle = hue(i * 40 + t * 10);
      ctx.fillRect(k.x - a, TITLE_H + k.y, 2 * a + 1, 1); ctx.fillRect(k.x, TITLE_H + k.y - a, 1, 2 * a + 1);
    });
    const unicorn = (ux, uy, f, t) => UNICORN_FRAMES[f].forEach((row, y) => {
      const mane = hue(y * 18 + t * 12);
      for (let x = 0; x < row.length; x++) { const k = row[x]; if (k === '.') continue; ctx.fillStyle = k === 'R' ? mane : css(to565(UNICORN_PALETTE[k])); ctx.fillRect(ux + x * UNI, TITLE_H + uy + y * UNI, UNI, UNI); }
    });
    // Unicorn at any scale, squashed horizontally (sx < 1) as it banks away from us.
    const unicornAt = (cx, top, sc, sx, f, t) => UNICORN_FRAMES[f].forEach((row, y) => {
      const mane = hue(y * 18 + t * 12), w = Math.max(1, Math.ceil(sc * sx)), h = Math.max(1, Math.ceil(sc));
      for (let x = 0; x < row.length; x++) { const k = row[x]; if (k === '.') continue; ctx.fillStyle = k === 'R' ? mane : css(to565(UNICORN_PALETTE[k])); ctx.fillRect(Math.round(cx + (x - 20) * sc * sx), Math.round(TITLE_H + top + y * sc), w, h); }
    });
    const titleDraw = t => {
      tctx.fillStyle = '#000'; tctx.fillRect(0, 0, 320, TITLE_H);
      let size = title.length * 18 > 316 ? 2 : 3;
      const cw = 6 * size, w = cw * title.length + 12, x0 = (320 - w) / 2 | 0, y0 = (TITLE_H - 8 * size) / 2 | 0;
      const glitch = rnd() % 7 === 0, jitter = glitch ? rnd() % 9 - 4 : 0;
      drawText(tctx, title, x0 - 2 + jitter, y0, size, c565(0, 255, 255));
      drawText(tctx, title, x0 + 2 - jitter, y0 + (glitch ? 1 : 0), size, c565(255, 0, 160));
      for (let i = 0; i < title.length; i++) drawText(tctx, title[i], x0 + i * cw, y0, size, hue(i * 22 - t * 9));
      drawText(tctx, 'TM', x0 + cw * title.length + 1, y0, 1, '#fff');
      if (glitch) { tctx.fillStyle = hue(rnd() % 360); tctx.fillRect(0, y0 + rnd() % (8 * size), 320, 2 + rnd() % 3); }
    };
    // Phases (fractions of duration): gallop, tremble during the loader's stall,
    // blast-off on a rainbow jet, then the jet floods the whole screen.
    const P_TREMBLE = 0.46, P_BLAST = 0.6, P_WASH = 0.86;
    const loading = (elapsed, t) => {
      const y = TITLE_H + SCENE_H, barX = 16, barY = y + 6, barW = 288, barH = 10;
      ctx.fillStyle = '#000'; ctx.fillRect(0, y, 320, 240 - y);
      if (!finale) {
        const k0 = elapsed / duration;
        const pct0 = k0 < 0.45 ? k0 / 0.45 * 64 | 0 : k0 < 0.62 ? 64 + ((k0 - 0.45) * 18 | 0) : 67 + ((k0 - 0.62) / 0.38 * 70 | 0);
        const dn0 = pct0 >= 100 ? barW - 2 : (barW - 2) * pct0 / 100 | 0;
        ctx.strokeStyle = c565(0, 255, 255); ctx.strokeRect(barX + 0.5, barY + 0.5, barW - 1, barH - 1);
        for (let x = 0; x < dn0; x++) { ctx.fillStyle = hue(x * 2 - t * 10); ctx.fillRect(barX + 1 + x, barY + 1, 1, barH - 2); }
        drawText(ctx, pct0 >= 100 ? tagline : LOADING[(k0 * 1.8 * LOADING.length | 0) % LOADING.length], barX, barY + barH + 6, 1, '#fff');
        drawText(ctx, pct0 >= 100 ? `${pct0}% (oops)` : `${pct0}%`, barX + barW, barY + barH + 6, 1, pct0 >= 100 ? hue(t * 20) : c565(255, 0, 200), true);
        return;
      }
      const k = elapsed / duration;
      const pct = k < 0.4 ? k / 0.4 * 64 | 0 : k < P_BLAST ? 64 + ((k - 0.4) * 14 | 0) : Math.min(137, 67 + ((k - P_BLAST) / (P_WASH - P_BLAST) * 70 | 0));
      const fill = barW - 2, dn = pct >= 100 ? fill : fill * pct / 100 | 0;
      ctx.strokeStyle = c565(0, 255, 255); ctx.strokeRect(barX + 0.5, barY + 0.5, barW - 1, barH - 1);
      for (let x = 0; x < dn; x++) { ctx.fillStyle = hue(x * 2 - t * 10); ctx.fillRect(barX + 1 + x, barY + 1, 1, barH - 2); }
      let text;
      if (k >= P_BLAST) text = k < P_BLAST + 0.03 ? 'Banking left...' : pct >= 100 ? 'BLAST OFF!!!' : 'Venting rainbows...';
      else if (k >= P_TREMBLE) text = (t >> 3) % 2 ? 'Uh oh. Rainbow pressure rising...' : 'Something is brewing...';
      else text = LOADING[(k * 1.8 * LOADING.length | 0) % LOADING.length];
      drawText(ctx, text, barX, barY + barH + 6, 1, k >= P_TREMBLE ? hue(t * 20) : '#fff');
      drawText(ctx, pct >= 100 ? `${pct}% (oops)` : `${pct}%`, barX + barW, barY + barH + 6, 1, pct >= 100 ? hue(t * 20) : c565(255, 0, 200), true);
    };
    // 4-point pixel sparkle.
    const star = (x, y, r, c) => { ctx.fillStyle = c; ctx.fillRect(x - r, y, 2 * r + 1, 1); ctx.fillRect(x, y - r, 1, 2 * r + 1); if (r > 2) { ctx.fillStyle = '#fff'; ctx.fillRect(x - 1, y - 1, 3, 3); } };
    const SPARK_C = ['#ffffff', c565(0, 255, 255), c565(255, 90, 200), c565(255, 220, 60)];
    let plume = [], jet = [];
    const snap = v => Math.round(v / 3) * 3;
    const drawPlume = t => {
      // Jet ribbon: six rainbow bands along the rear's path, fattening with age.
      for (let j = 0; j < jet.length; j++) {
        const p = jet[j], age = jet.length - j, w = Math.min(60, p.sc * 2 * (1 + age * 0.12));
        for (let b = 0; b < 6; b++) { ctx.fillStyle = hue(b * 60 - t * 10); ctx.fillRect(snap(p.x - w), TITLE_H + snap(p.y + (b - 3) * w * 0.9), snap(w * 2) + 3, Math.ceil(w * 0.9) + 1); }
      }
      // Blobs: chunky rainbow pixels that spray back, swell and pile up.
      for (const q of plume) {
        const z = q.s / 3; q.x += q.vx * z; q.y += q.vy * z; if (q.s < q.max) q.s *= q.g;
        ctx.fillStyle = hue(q.h - t * 6); const s = snap(q.s) || 3; ctx.fillRect(snap(q.x), TITLE_H + snap(q.y), s, s);
      }
      for (const q of plume) if ((q.id + t) % 7 === 0) star(snap(q.x) + 1, TITLE_H + snap(q.y) + 1, 2 + (q.id % 3), SPARK_C[(q.id + (t >> 2)) % 4]);
    };
    const wash = (w, t) => {
      // The flood: a diagonal rainbow wipe from bottom-left that eats the screen.
      const front = w * 300 - 30;
      for (let y = 0; y < 240; y += 6) for (let x = 0; x < 320; x += 6) {
        const d = 240 - y;
        if (d > front + Math.sin((x + t * 6) / 16) * 14) continue;
        ctx.fillStyle = hue((x + y) * 1.2 - t * 14); ctx.fillRect(x, y, 6, 6);
      }
      for (let i = 0; i < 40; i++) { const x = (i * 97 + t * 13) % 320, y = (i * 61 + t * 7) % 240; if (240 - y < front && ((i + (t >> 1)) % 3)) star(x, y, 2 + (i % 4), SPARK_C[i % 4]); }
      if (w >= 1) {
        const msg = tagline, sz = msg.length * 12 > 300 ? 1 : 2, x0 = (320 - msg.length * 6 * sz) / 2 | 0, y0 = 112;
        drawText(ctx, msg, x0 - 2, y0, sz, c565(0, 255, 255)); drawText(ctx, msg, x0 + 2, y0, sz, c565(255, 0, 160)); drawText(ctx, msg, x0, y0, sz, '#fff');
      }
    };
    const frame = now => {
      raf = requestAnimationFrame(frame);
      if (now - last < 40) return; last = now;
      let elapsed = now - start;
      if (elapsed >= duration) { if (!done && onDone) onDone(); if (!loop) { done = true; return; } start = now; elapsed = 0; t = 0; sparks.forEach(s => (s.life = 0)); plume = []; jet = []; }
      const k = finale ? elapsed / duration : (elapsed / duration) * P_TREMBLE / 0.999;
      let ux = -110 + Math.min(t * 6, 170), uy = 22 + (3 * Math.sin(t / 2) | 0), f = (t / 4 | 0) % 2, shake = 0;
      if (k >= P_TREMBLE && k < P_BLAST) {
        const n = (k - P_TREMBLE) / (P_BLAST - P_TREMBLE), amp = 1 + n * 4;
        ux += (rnd() % 3 - 1) * amp; uy = 22 + (rnd() % 3 - 1) * amp; f = 0; shake = n > 0.6 ? 1 : 0;
      }
      // Blast-off: bank left (squash) to face the horizon, then rocket up and away
      // over the sun, shrinking into the distance, spraying rainbow at the viewer.
      let far = null;
      if (k >= P_BLAST) {
        const n = Math.min(1, (k - P_BLAST) / (P_WASH - P_BLAST));
        let cx = 120, top = 22, sc = 3, sx = 1;
        if (n < 0.16) { const tn = n / 0.16; sx = 1 - 0.55 * tn; top = 22 - tn * 6; shake = 2; f = 0; }
        else {
          const m = (n - 0.16) / 0.84, e = 1 - (1 - m) * (1 - m);
          sx = 0.45; sc = 3 - 2.55 * e; cx = 120 + (226 - 120) * e;
          top = 16 + (26 - 16) * e - Math.sin(m * Math.PI) * 18 - sc * 6; f = (t / 2 | 0) % 2; shake = sc > 1.5 ? 3 : 1;
        }
        far = { cx, top, sc, sx, n };
        const rx = cx, ry = top + 18 * sc;
        if (n >= 0.1) {
          jet.push({ x: rx, y: ry, sc }); if (jet.length > 70) jet.shift();
          const burst = 14 + (n * 26 | 0);
          for (let i = 0; i < burst && plume.length < 1400; i++) plume.push({ id: plume.length, x: rx + (rnd() % 5 - 2) * sc, y: ry, vx: (rnd() % 13 - 6) * 0.45, vy: (rnd() % 11 - 2) * 0.45, s: Math.max(1.5, sc), g: 1.05 + (rnd() % 5) * 0.01, max: 24 + rnd() % 24, h: (plume.length * 11) % 360 });
        }
      }
      if (shake) ctx.setTransform(1, 0, 0, 1, (rnd() % (2 * shake + 1)) - shake, (rnd() % (2 * shake + 1)) - shake);
      sky(t); grid(t);
      if (k < P_BLAST) trail(ux, uy, t);
      if (k < P_TREMBLE) sparksDraw(ux, uy, t);
      if (far) { if (far.n < 0.97) unicornAt(far.cx, far.top, far.sc, far.sx, f, t); else star(far.cx | 0, TITLE_H + (far.top | 0) + 6, 6, SPARK_C[t % 4]); }
      else unicorn(ux | 0, uy | 0, f, t);
      if (k >= P_BLAST) drawPlume(t);
      if (k >= P_TREMBLE && k < P_BLAST && (t >> 2) % 2) { drawText(ctx, '!!', ux + 34 * UNI, TITLE_H + uy - 18, 2, c565(255, 0, 200)); ctx.fillStyle = c565(0, 255, 255); ctx.fillRect(ux + 26 * UNI + (t % 3), TITLE_H + uy + 4 + (t % 6) * 2, 2, 3); }
      if (t % 3 === 0 || shake) titleDraw(t);
      ctx.drawImage(titleCv, 0, 0);
      loading(elapsed, t);
      if (k >= P_WASH - 0.04) wash(Math.min(1, (k - P_WASH + 0.04) / 0.12), t);
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      t++;
    };
    ctx.fillStyle = '#000'; ctx.fillRect(0, 0, 320, 240);
    (document.fonts ? document.fonts.load("8px 'Press Start 2P'") : Promise.resolve()).finally(() => { raf = requestAnimationFrame(frame); });
    return () => cancelAnimationFrame(raf);
  }, [title, tagline, duration, loop, finale]);
  return React.createElement('canvas', { ref, width: 320, height: 240, style: { width: 320 * scale, height: 240 * scale, imageRendering: 'pixelated', display: 'block', background: '#000', ...style } });
}

