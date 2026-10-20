// CyberUnicorn 5000 app icons — source of truth.
// Builds every icon as a 32x24 character grid on the boot unicorn's palette
// (make_unicorn.py), plus four animated indices. Run by the project's build
// script to regenerate components/icons/iconData.js and icons/cyber_icons.h.
// Hand-tune by editing the shape calls below, never the generated files.
function buildIcons() {
  const W = 32, H = 24;
  // key: [index, rgb | special]. 0-9 are identical to unicorn_sprite.h.
  const PALETTE = [
    ['.', null, 'transparent'],
    ['R', 'rainbow', 'animated rainbow (hue by icon mode + time)'],
    ['K', [24, 6, 40], 'outline, deep purple'],
    ['W', [255, 255, 255], 'body white'],
    ['w', [200, 180, 255], 'body shade, lavender'],
    ['H', [255, 220, 60], 'horn / sun yellow'],
    ['h', [255, 140, 0], 'horn shade, orange'],
    ['E', [0, 255, 255], 'cyber eye, neon cyan'],
    ['P', [255, 90, 200], 'hot pink'],
    ['G', [0, 255, 160], 'neon green (hooves)'],
    ['M', [255, 0, 200], 'magenta (grid rays, title shadow)'],
    ['B', [0, 160, 255], 'grid blue'],
    ['D', [30, 0, 70], 'sky top / panel'],
    ['V', [96, 0, 48], 'sky low / dim LED'],
    ['N', [12, 0, 28], 'grid floor / screen'],
    ['o', [255, 150, 120], 'sun mid'],
    ['S', 'sparkle', 'twinkle: white, cyan, pink, off'],
    ['A', 'frameA', 'rainbow, frame A only'],
    ['a', 'frameB', 'rainbow, frame B only'],
    ['Q', 'gridA', 'grid blue, frame A only'],
    ['q', 'gridB', 'grid blue, frame B only'],
  ];
  const icons = {};
  function make(id, name, mode, draw, note) {
    const g = Array.from({ length: H }, () => Array(W).fill('.'));
    const T = {
      p(x, y, c) { x = Math.round(x); y = Math.round(y); if (x >= 0 && y >= 0 && x < W && y < H) g[y][x] = c; },
      get(x, y) { return x >= 0 && y >= 0 && x < W && y < H ? g[y][x] : '.'; },
      rect(x0, y0, x1, y1, c) { for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) T.p(x, y, c); },
      box(x0, y0, x1, y1, c) { for (let x = x0; x <= x1; x++) { T.p(x, y0, c); T.p(x, y1, c); } for (let y = y0; y <= y1; y++) { T.p(x0, y, c); T.p(x1, y, c); } },
      rrect(x0, y0, x1, y1, c, r = 1) {
        for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) {
          const dx = x < x0 + r ? x0 + r - x : x > x1 - r ? x - (x1 - r) : 0;
          const dy = y < y0 + r ? y0 + r - y : y > y1 - r ? y - (y1 - r) : 0;
          if (dx * dx + dy * dy <= r * r) T.p(x, y, c);
        }
      },
      hl(x0, y, x1, c) { for (let x = x0; x <= x1; x++) T.p(x, y, c); },
      vl(x, y0, y1, c) { for (let y = y0; y <= y1; y++) T.p(x, y, c); },
      disc(cx, cy, r, c, fn) { for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) { const d = (x - cx) ** 2 + (y - cy) ** 2; if (d <= r * r + r * 0.6 && (!fn || fn(x, y))) T.p(x, y, c); } },
      ring(cx, cy, r0, r1, c, a0 = -180, a1 = 180) {
        for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
          const d = Math.hypot(x - cx, y - cy);
          let a = Math.atan2(y - cy, x - cx) * 180 / Math.PI;
          const inA = a0 <= a1 ? a >= a0 && a <= a1 : a >= a0 || a <= a1;
          if (d >= r0 - 0.5 && d < r1 + 0.5 && inA) T.p(x, y, c);
        }
      },
      line(x0, y0, x1, y1, c) {
        const n = Math.max(Math.abs(x1 - x0), Math.abs(y1 - y0), 1);
        for (let i = 0; i <= n; i++) T.p(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, c);
      },
      // Unicorn-style chunky outline: K on every empty cell touching `chars`.
      outline(chars, c = 'K') {
        const add = [];
        for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
          if (g[y][x] !== '.') continue;
          if ([[1, 0], [-1, 0], [0, 1], [0, -1]].some(([dx, dy]) => chars.includes(T.get(x + dx, y + dy)))) add.push([x, y]);
        }
        add.forEach(([x, y]) => (g[y][x] = c));
      },
      glyph(rows, x0, y0, c) { rows.forEach((r, y) => [...r].forEach((ch, x) => ch === 'X' && T.p(x0 + x, y0 + y, c))); },
    };
    draw(T);
    icons[id] = { id, name, mode, note, grid: g.map(r => r.join('')) };
  }

  // ---- CyberUnicorn apps (apps/apps.h order) ----
  // The Frequency logo, traced from uploads/Frequency-Logo-Round-Icon.png at 24 px:
  // ring + FR ligature (F stem and top bar running into the R bowl, short F
  // crossbar, R stem and diagonal leg). Synthwave version: rainbow ring, night-sky
  // disc, white FR, striped sun rising behind the legs.
  make('frequency', 'Frequency Check-in', 'angle', T => {
    const cx = 15.5, cy = 11.5, d = (x, y) => Math.hypot(x - cx, y - cy);
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) if (d(x, y) < 9.7) T.p(x, y, y < 8 ? 'D' : y < 14 ? 'V' : 'N');
    // striped sun rising from the bottom of the disc
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) {
      if (d(x, y) >= 9.7 || Math.hypot(x - 15.5, y - 21) > 7.2 || y > 20) continue;
      if (y === 17 || y === 19) continue;
      T.p(x, y, y <= 15 ? 'H' : y <= 16 ? 'h' : y <= 18 ? 'o' : 'P');
    }
    [[10, 4], [21, 3], [24, 9], [7, 10]].forEach(([x, y]) => T.p(x, y, 'S'));
    const FR = [
      '.XXXXXXXXXXX...',
      'XXXXXXXXXXXXX..',
      'XX.........XXX.',
      'XX..........XX.',
      'XXXXX.XXXXXXXX.',
      'XXXXX.XXXXXXX..',
      'XX....XX..XX...',
      'XX....XX...XX..',
      'XX....XX...XX..',
      'XX....XX....XX.',
      'XX....XX.....XX',
    ];
    const ox = 8, oy = 6;
    FR.forEach((r, j) => [...r].forEach((c, i) => c === 'X' && T.p(ox + i, oy + j, j >= 9 ? 'w' : 'W')));
    T.ring(cx, cy, 10, 10, 'K');
    T.ring(cx, cy, 11, 11.6, 'R');
    T.p(2, 2, 'S'); T.p(29, 3, 'S'); T.p(29, 21, 'S'); T.p(2, 21, 'S');
    [[3, 6], [28, 16]].forEach(([x, y], i) => { const c = i ? 'a' : 'A'; T.p(x, y, c); T.p(x - 1, y, c); T.p(x + 1, y, c); T.p(x, y - 1, c); T.p(x, y + 1, c); });
  }, 'the Frequency FR logo: rainbow ring, white FR on a night-sky disc, striped sun rising');

  make('lightshow', 'LED Light Show', 'diag', T => {
    const heart = ['.XX..XX.', 'XXXXXXXX', 'XXXXXXXX', '.XXXXXX.', '..XXXX..', '...XX...', '........'];
    T.rrect(3, 0, 28, 21, 'D', 1);
    const on = (c, r) => heart[r] && heart[r][c] === 'X';
    for (let r = 0; r < 7; r++) for (let c = 0; c < 8; c++) {
      const x = 5 + c * 3, y = 1 + r * 3;
      let ch = 'N';
      if (on(c, r)) ch = [[1, 0], [-1, 0], [0, 1], [0, -1]].every(([dx, dy]) => on(c + dx, r + dy)) ? 'R' : 'A';
      T.rect(x, y, x + 1, y + 1, ch);
    }
    T.p(26, 19, 'S'); T.p(5, 19, 'S');
    T.outline(['D']); T.rect(14, 23, 17, 23, 'w');
  }, '8x8 LED panel; rainbow heart beats');

  make('sensors', 'Sensor Hub', 'col', T => {
    T.rrect(1, 1, 30, 20, 'w', 2); T.rect(3, 3, 28, 18, 'N');
    [7, 11, 15].forEach(y => T.hl(3, y, 28, 'D')); [8, 13, 18, 23].forEach(x => T.vl(x, 3, 18, 'D'));
    const ys = [15, 11, 13, 7, 10, 5, 8, 6];
    for (let i = 1; i < ys.length; i++) { const x0 = 4 + (i - 1) * 3, x1 = 4 + i * 3; T.line(x0, ys[i - 1], x1, ys[i], 'R'); T.line(x0, ys[i - 1] + 1, x1, ys[i] + 1, 'R'); }
    T.rect(25, 5, 26, 7, 'A');
    T.p(25, 15, 'G'); T.p(27, 15, 'G'); T.p(25, 17, 'a'); T.p(27, 17, 'a');
    T.rect(12, 21, 19, 22, 'w'); T.outline(['w']);
  }, 'little screen with a rainbow sparkline and node dots');

  make('led', 'RGB LED', 'row', T => {
    T.disc(16, 9, 6, 'R', (x, y) => y <= 9); T.rect(10, 9, 22, 14, 'R');
    T.vl(12, 6, 11, 'W'); T.p(13, 4, 'W'); T.p(13, 5, 'W');
    T.rect(9, 15, 23, 16, 'w');
    T.vl(13, 17, 23, 'w'); T.vl(19, 17, 21, 'w'); T.p(20, 21, 'w');
    T.outline(['R', 'W', 'w']);
    for (let k = 0; k < 8; k++) {
      const a = -180 + k * 45; if (a > 10 && a < 170) continue;
      const c = k % 2 ? 'a' : 'A'; const r = Math.PI / 180;
      for (let d = 10; d <= 12; d++) T.p(16 + Math.cos(a * r) * d, 9 + Math.sin(a * r) * d, c);
    }
    T.p(3, 20, 'S'); T.p(28, 18, 'S');
  }, '5 mm LED: rainbow dome, glow rays');

  make('light', 'Light Meter', 'angle', T => {
    T.disc(16, 12, 8, 'H');
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) if (T.get(x, y) === 'H') { if (y >= 7) T.p(x, y, 'h'); if (y >= 11) T.p(x, y, 'o'); if (y >= 15) T.p(x, y, 'P'); }
    [13, 16, 19].forEach(y => T.hl(0, y, 31, '.'));
    T.outline(['H', 'h', 'o', 'P']);
    for (let k = 0; k < 12; k++) {
      const a = k * 30 * Math.PI / 180, c = k % 2 ? 'a' : 'A';
      for (let d = 11; d <= 13; d++) T.p(16 + Math.cos(a) * d, 12 + Math.sin(a) * d, c);
    }
  }, 'the boot screen\'s striped synthwave sun; rays spin');

  make('sound', 'Sound & Tones', 'row', T => {
    T.rect(3, 9, 7, 14, 'w');
    for (let i = 0; i < 6; i++) T.vl(8 + i, 9 - i, 14 + i, 'W');
    T.vl(14, 3, 20, 'w'); T.vl(9, 9, 14, 'w');
    T.outline(['W', 'w']);
    T.ring(13, 11.5, 5, 6, 'R', -40, 40); T.ring(13, 11.5, 9, 10, 'A', -45, 45); T.ring(13, 11.5, 13, 14, 'a', -48, 48);
    T.p(29, 2, 'S'); T.p(30, 21, 'S');
  }, 'speaker cone, rainbow waves pulse out');

  make('slideshow', 'Photo Slideshow', 'angle', T => {
    T.box(0, 0, 31, 23, 'R'); T.box(1, 1, 30, 22, 'R');
    T.rect(2, 2, 29, 7, 'D'); T.rect(2, 8, 29, 12, 'V');
    T.disc(21, 13, 6, 'H', (x, y) => y <= 12 && y >= 2);
    for (let y = 2; y <= 12; y++) for (let x = 2; x < 30; x++) if (T.get(x, y) === 'H') { if (y >= 9) T.p(x, y, 'h'); if (y >= 11) T.p(x, y, 'P'); }
    [10, 12].forEach(y => { for (let x = 14; x < 29; x++) if ('Hh P'.includes(T.get(x, y)) && T.get(x, y) !== ' ') T.p(x, y, 'V'); });
    T.p(5, 4, 'S'); T.p(10, 3, 'S'); T.p(14, 6, 'S'); T.p(27, 4, 'S');
    T.rect(2, 14, 29, 21, 'N');
    for (let k = -4; k <= 4; k++) T.line(16 + k, 13, 16 + k * 5, 21, 'M');
    [15, 17, 20].forEach(y => T.hl(2, y, 29, 'B'));
    T.hl(2, 13, 29, 'M');
  }, 'rainbow picture frame around a tiny boot scene');

  make('weather', 'Weather Forecast', 'col', T => {
    T.disc(21, 7, 5, 'H'); for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) if (T.get(x, y) === 'H' && y >= 8) T.p(x, y, 'h');
    T.outline(['H', 'h']);
    T.disc(11, 13, 4, 'W'); T.disc(17, 11, 5, 'W'); T.disc(23, 14, 3, 'W'); T.rect(8, 13, 25, 17, 'W');
    T.hl(8, 17, 25, 'w'); T.hl(10, 16, 13, 'w'); T.p(26, 15, 'w');
    T.outline(['W', 'w']);
    [[29, 3], [27, 0], [30, 8]].forEach(([x, y], i) => T.p(x, y, i % 2 ? 'a' : 'A'));
    [9, 12, 15, 18, 21, 24].forEach((x, i) => { const o = i % 2; T.vl(x, 19 + o, 20 + o, o ? 'a' : 'A'); T.vl(x - 1, 21 + o, 22 + o, o ? 'A' : 'a'); });
  }, 'sun behind a cloud, rainbow rain falls');

  make('hat', 'Hat Module Probe', 'row', T => {
    T.rrect(1, 5, 30, 19, 'D', 1);
    for (let x = 3; x <= 28; x += 2) { T.p(x, 4, 'H'); T.p(x, 3, 'h'); }
    T.hl(3, 18, 28, 'V'); T.vl(3, 7, 18, 'V'); T.vl(28, 7, 18, 'V');
    T.rect(6, 8, 25, 16, 'M');
    for (let i = 0; i < 6; i++) {
      const sx = 8 + i * 3; T.rect(sx, 9, sx + 1, 14, 'N');
      if (i === 2) { T.rect(sx, 9, sx + 1, 11, 'A'); T.rect(sx, 12, sx + 1, 14, 'a'); }
      else T.rect(sx, 12, sx + 1, 14, 'W');
      T.p(sx, 16, 'W');
    }
    T.outline(['D', 'H', 'h']);
  }, 'the NM-RF-HAT with its DIP switch; DIP 3 (PN532) flips');

  make('gpio', 'GPIO Pins', 'row', T => {
    T.rect(9, 2, 22, 21, 'D'); T.box(9, 2, 22, 21, 'w'); T.hl(10, 3, 21, 'V');
    T.rect(11, 4, 12, 5, 'P');
    const three = ['XXX', '..X', 'XXX', '..X', 'XXX'], two = ['XXX', '..X', 'XXX', 'X..', 'XXX'];
    T.glyph(three, 12, 10, 'E'); T.glyph(two, 16, 10, 'E');
    [3, 7, 11, 15, 19].forEach((y, i) => { T.rect(3, y, 8, y + 1, 'R'); T.rect(23, y, 28, y + 1, 'R'); T.rect(1, y, 2, y + 1, i % 2 ? 'a' : 'A'); T.rect(29, y, 30, y + 1, i % 2 ? 'A' : 'a'); });
    T.outline(['w', 'R']);
  }, 'ESP32 chip with rainbow legs; signals hop pin to pin');

  make('remote', 'Phone Remote', 'row', T => {
    T.rrect(2, 1, 13, 22, 'w', 2); T.rect(4, 4, 11, 17, 'N'); T.rect(4, 6, 11, 15, 'R');
    T.hl(6, 2, 9, 'D'); T.rect(7, 19, 8, 20, 'D');
    T.outline(['w']);
    [[17, 3], [20, 6], [23, 9], [26, 12]].forEach(([x, h], i) => { T.rect(x, 18 - h, x + 1, 18, i < 3 ? 'R' : 'a'); });
    T.outline(['R'].concat([]));
    T.p(29, 2, 'S'); T.p(22, 3, 'S');
  }, 'phone with a rainbow screen; signal bars fill');

  make('spectrum', 'Spectrum', 'col', T => {
    T.rrect(1, 1, 30, 20, 'N', 2);
    // analyser bars: solid rainbow body, A/a tops bounce between two heights
    const lo = [9, 13, 7, 11, 5, 10, 14], hi = [6, 9, 4, 13, 8, 6, 11];
    lo.forEach((top, i) => {
      const x = 4 + i * 4, t0 = Math.min(top, hi[i]), t1 = Math.max(top, hi[i]);
      T.rect(x, t1, x + 2, 17, 'R');
      T.rect(x, t0, x + 2, t1 - 1, top < hi[i] ? 'A' : 'a');
      T.hl(x, t0 - 2, x + 2, 'W');
    });
    T.hl(3, 18, 28, 'w');
    T.outline(['N']);
    T.rect(12, 21, 19, 22, 'w');
    T.p(29, 2, 'S'); T.p(2, 22, 'S');
  }, 'little analyser screen; rainbow bars bounce to the beat');

  // ---- Bruce's built-in menus (re-skinned) ----
  make('files', 'Files', 'row', T => {
    T.rrect(3, 3, 13, 7, 'h', 1);
    T.rrect(3, 5, 28, 20, 'H', 1);
    T.hl(4, 20, 27, 'h');
    T.rect(6, 9, 25, 10, 'R'); T.rect(6, 13, 21, 14, 'R'); T.rect(6, 17, 17, 17, 'A');
    T.outline(['H', 'h']);
    T.p(29, 2, 'S'); T.p(1, 22, 'S');
  }, 'golden folder with rainbow file lines; the last one blinks');
  make('library', 'Library', 'col', T => {
    T.rect(2, 20, 29, 21, 'w'); T.hl(2, 29, 22, 'K');
    T.rect(4, 6, 7, 19, 'R'); T.rect(9, 3, 12, 19, 'R'); T.rect(14, 8, 16, 19, 'R');
    T.hl(4, 7, 9, 'W'); T.hl(9, 12, 6, 'W'); T.hl(14, 16, 11, 'W');
    for (let i = 0; i < 12; i++) T.rect(19 + Math.floor(i / 3), 19 - i, 21 + Math.floor(i / 3), 19 - i, 'H');
    T.p(20, 17, 'h'); T.p(23, 9, 'h');
    T.rect(25, 10, 27, 19, 'A');
    T.outline(['R', 'H', 'h', 'w', 'A']);
    T.p(29, 3, 'S'); T.p(1, 2, 'S');
  }, 'shelf of rainbow book spines, a golden book leaning, the last spine blinks');
  make('wifi', 'WiFi Tools', 'row', T => {
    T.rect(15, 19, 17, 21, 'W'); T.outline(['W']);
    T.ring(16, 20, 4, 5, 'R', -140, -40); T.ring(16, 20, 8, 9, 'R', -140, -40);
    T.ring(16, 20, 12, 13, 'A', -140, -40); T.ring(16, 20, 16, 17, 'a', -140, -40);
    T.p(3, 3, 'S'); T.p(29, 20, 'S');
  }, 'fan of arcs, outer two pulse');

  make('ble', 'Bluetooth LE', 'angle', T => {
    T.rrect(5, 4, 26, 19, 'B', 4); T.hl(7, 18, 24, 'D');
    T.glyph(['XXX.', 'X..X', 'XXX.', 'X..X', 'X..X', 'XXX.'], 8, 8, 'W');
    T.glyph(['X...', 'X...', 'X...', 'X...', 'X...', 'XXX.'], 13, 8, 'W');
    T.glyph(['XXX', 'X..', 'XX.', 'X..', 'X..', 'XXX'], 18, 8, 'W');
    T.rect(22, 7, 23, 8, 'A');
    T.outline(['B', 'D']);
    T.ring(16, 12, 11, 11, 'A', 150, -150); T.ring(16, 12, 11, 11, 'A', -30, 30);
    T.ring(16, 12, 14, 14, 'a', 150, -150); T.ring(16, 12, 14, 14, 'a', -30, 30);
  }, 'blue "BLE" badge with broadcast arcs (no Bluetooth rune: trademark)');

  make('rf', 'Sub-GHz Radio', 'row', T => {
    T.rect(15, 9, 16, 19, 'w'); T.hl(13, 20, 18, 'w'); T.hl(12, 21, 19, 'w'); T.hl(11, 22, 20, 'w');
    T.rrect(13, 4, 18, 8, 'P', 1);
    T.outline(['w', 'P']);
    [[5, 6, 'R'], [9, 10, 'A'], [13, 14, 'a']].forEach(([r0, r1, c]) => { T.ring(15.5, 6, r0, r1, c, -45, 45); T.ring(15.5, 6, r0, r1, c, 135, -135); });
  }, 'Sub-GHz antenna mast radiating both ways');

  make('rfid', 'RFID Cards', 'col', T => {
    T.rrect(1, 4, 25, 20, 'W', 2); T.hl(3, 20, 23, 'w'); T.vl(25, 6, 18, 'w');
    T.rect(4, 8, 10, 13, 'H'); T.hl(4, 10, 10, 'h'); T.vl(7, 8, 13, 'h');
    T.hl(13, 9, 22, 'w'); T.hl(13, 12, 19, 'w');
    T.rect(2, 16, 24, 17, 'R');
    T.outline(['W', 'w']);
    T.ring(22, 12, 6, 6, 'A', -35, 35); T.ring(22, 12, 8, 8, 'a', -40, 40);
  }, 'card with a gold chip and holo stripe');

  make('ir', 'Infrared Remote', 'col', T => {
    T.rrect(1, 8, 15, 16, 'w', 2);
    T.rect(4, 10, 5, 11, 'P'); T.rect(8, 10, 9, 11, 'E'); T.rect(4, 13, 5, 14, 'G'); T.rect(8, 13, 9, 14, 'H'); T.rect(12, 11, 13, 13, 'D');
    T.rect(16, 11, 17, 13, 'M');
    T.outline(['w', 'M']);
    for (let k = 0; k < 4; k++) { const x = 20 + k * 3, c = k % 2 ? 'a' : 'A'; for (let y = 12 - k; y <= 12 + k; y++) if (y === 12 - k || y === 12 + k || y === 12) { T.p(x, y, c); T.p(x + 1, y, c); } }
  }, 'remote firing a rainbow beam');

  make('nrf24', '2.4 GHz Radio', 'row', T => {
    T.rrect(2, 8, 29, 21, 'D', 1);
    T.hl(4, 10, 10, 'H'); T.vl(10, 10, 13, 'H'); T.hl(4, 13, 10, 'H'); T.vl(4, 13, 16, 'H'); T.hl(4, 16, 10, 'H'); T.vl(10, 16, 19, 'H'); T.hl(4, 19, 10, 'H');
    T.rect(14, 11, 20, 17, 'N'); T.p(15, 12, 'E'); T.rect(14, 19, 17, 19, 'w');
    for (let y = 10; y <= 18; y += 2) { T.p(24, y, 'H'); T.p(26, y, 'H'); }
    T.outline(['D']);
    T.ring(7, 8, 3, 3, 'R', -150, -30); T.ring(7, 8, 5, 5, 'A', -150, -30); T.ring(7, 8, 7, 7, 'a', -150, -30);
  }, '2.4 GHz module with meander antenna');

  make('gps', 'GPS & Wardriving', 'col', T => {
    for (let k = -5; k <= 5; k++) T.line(16 + k * 2, 14, 16 + k * 6, 23, 'M');
    [15, 17, 20, 23].forEach(y => T.hl(0, y, 31, 'B'));
    for (let a = 0; a < 360; a += 6) { const r = a * Math.PI / 180; T.p(16 + Math.cos(r) * 6, 20 + Math.sin(r) * 2, 'A'); T.p(16 + Math.cos(r) * 10, 20 + Math.sin(r) * 3, 'a'); }
    T.disc(16, 7, 6, 'P');
    for (let y = 8; y <= 19; y++) { const hw = Math.round(6 * (19 - y) / 11); T.hl(16 - hw, y, 16 + hw, 'P'); }
    T.disc(16, 7, 2, 'W'); T.p(12, 4, 'w'); T.p(11, 5, 'w'); T.p(11, 6, 'w');
    T.outline(['P']);
  }, 'map pin landing on the synthwave grid');

  make('scripts', 'JS Scripts', 'col', T => {
    T.rect(1, 2, 30, 21, 'N'); T.box(1, 2, 30, 21, 'w'); T.rect(2, 3, 29, 4, 'R');
    T.glyph(['X..', '.X.', '..X', '.X.', 'X..'], 3, 7, 'G');
    T.hl(8, 9, 14, 'E'); T.hl(16, 9, 22, 'w'); T.hl(4, 13, 9, 'P'); T.hl(11, 13, 20, 'w'); T.hl(4, 16, 10, 'H');
    T.rect(12, 15, 13, 17, 'A');
    T.outline(['w']);
  }, 'terminal with rainbow title bar; cursor blinks');

  make('others', 'More Tools', 'angle', T => {
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) { const dx = Math.abs(x - 16), dy = Math.abs(y - 12); if (Math.sqrt(dx) + Math.sqrt(dy) <= Math.sqrt(11)) T.p(x, y, 'R'); }
    T.p(16, 12, 'W'); T.p(15, 12, 'W'); T.p(16, 11, 'W');
    T.outline(['R', 'W']);
    [[5, 4], [27, 19], [26, 4], [5, 19]].forEach(([x, y], i) => { const c = i % 2 ? 'a' : 'A'; T.p(x, y, c); T.p(x - 1, y, c); T.p(x + 1, y, c); T.p(x, y - 1, c); T.p(x, y + 1, c); });
    T.p(10, 2, 'S'); T.p(23, 21, 'S');
  }, 'four-point rainbow sparkle');

  make('clock', 'Desk Clock', 'angle', T => {
    T.disc(16, 12, 10, 'R'); T.disc(16, 12, 8, 'W');
    [[16, 5], [23, 12], [16, 19], [9, 12]].forEach(([x, y]) => T.p(x, y, 'P'));
    T.line(16, 12, 16, 7, 'K'); T.line(16, 12, 20, 14, 'K');
    T.line(16, 12, 12, 8, 'A'); T.line(16, 12, 21, 9, 'a');
    T.p(16, 12, 'P');
    T.outline(['R']);
  }, 'rainbow-rimmed clock; second hand ticks');

  make('connect', 'File Share', 'row', T => {
    [[1, 'R'], [20, 'E']].forEach(([x0, c], i) => {
      T.rect(x0, 6, x0 + 10, 16, 'H'); T.hl(x0, 16, x0 + 10, 'h'); T.rect(x0 + 1, 7, x0 + 9, 14, 'N');
      if (i === 0) T.rect(x0 + 2, 9, x0 + 8, 12, 'R'); else { T.rect(x0 + 3, 8, x0 + 6, 13, 'W'); T.hl(x0 + 4, 10, x0 + 5, 'w'); T.hl(x0 + 4, 11, x0 + 5, 'w'); }
    });
    T.outline(['H', 'h']);
    T.hl(13, 9, 17, 'A'); T.p(16, 8, 'A'); T.p(16, 10, 'A');
    T.hl(14, 13, 18, 'a'); T.p(15, 12, 'a'); T.p(15, 14, 'a');
    T.p(3, 2, 'S'); T.p(28, 21, 'S');
  }, 'two Cheap Yellow Displays swapping a file');

  make('config', 'Settings', 'angle', T => {
    T.disc(16, 12, 7, 'w'); T.ring(16, 12, 4, 5, 'R'); T.disc(16, 12, 2, '.');
    T.p(12, 8, 'W'); T.p(13, 7, 'W'); T.p(11, 9, 'W');
    T.outline(['w']);
    for (let k = 0; k < 8; k++) {
      const a = k * 45 * Math.PI / 180;
      for (let d = 7; d <= 10; d++) for (let s = -1.2; s <= 1.2; s += 0.4) T.p(16 + Math.cos(a) * d - Math.sin(a) * s, 12 + Math.sin(a) * d + Math.cos(a) * s, 'w');
    }
    T.ring(16, 12, 4, 5, 'R'); T.p(12, 8, 'W'); T.p(13, 7, 'W'); T.p(11, 9, 'W');
    T.outline(['w']);
    for (let k = 0; k < 8; k++) { const a = (k * 45 + 22.5) * Math.PI / 180; T.p(16 + Math.cos(a) * 12, 12 + Math.sin(a) * 12, k % 2 ? 'a' : 'A'); }
    T.disc(16, 12, 2, 'K', (x, y) => Math.hypot(x - 16, y - 12) > 1.5); T.disc(16, 12, 1, '.');
  }, 'gear with a rainbow hub; sparks orbit');

  return { W, H, PALETTE, icons };
}
if (typeof module !== 'undefined') module.exports = { buildIcons };
