/* @ds-bundle: {"format":4,"namespace":"CyberUnicorn5000DesignSystem_f09c19","components":[{"name":"StatusBar","sourcePath":"components/device/AppFrame.jsx"},{"name":"AppFrame","sourcePath":"components/device/AppFrame.jsx"},{"name":"BootScene","sourcePath":"components/device/BootScene.jsx"},{"name":"DeviceScreen","sourcePath":"components/device/DeviceScreen.jsx"},{"name":"MenuScreen","sourcePath":"components/device/MenuScreen.jsx"},{"name":"OptionList","sourcePath":"components/device/OptionList.jsx"},{"name":"ScreenText","sourcePath":"components/device/ScreenText.jsx"},{"name":"PixelIcon","sourcePath":"components/icons/PixelIcon.jsx"},{"name":"UnicornSprite","sourcePath":"components/icons/UnicornSprite.jsx"},{"name":"ICON_W","sourcePath":"components/icons/iconData.js"},{"name":"ICON_H","sourcePath":"components/icons/iconData.js"},{"name":"PALETTE","sourcePath":"components/icons/iconData.js"},{"name":"ICONS","sourcePath":"components/icons/iconData.js"},{"name":"ICON_ORDER","sourcePath":"components/icons/iconData.js"},{"name":"UNICORN_PALETTE","sourcePath":"components/icons/unicornData.js"},{"name":"UNICORN_FRAMES","sourcePath":"components/icons/unicornData.js"},{"name":"RainbowBar","sourcePath":"components/rainbow/RainbowBar.jsx"},{"name":"RainbowFrame","sourcePath":"components/rainbow/RainbowFrame.jsx"},{"name":"RainbowText","sourcePath":"components/rainbow/RainbowText.jsx"}],"sourceHashes":{"components/device/AppFrame.jsx":"8e50feb1074e","components/device/BootScene.jsx":"8a956b8aa8a3","components/device/DeviceScreen.jsx":"82afb72ea284","components/device/MenuScreen.jsx":"567ffa2337d9","components/device/OptionList.jsx":"036c7377171d","components/device/ScreenText.jsx":"9816073f05bb","components/icons/PixelIcon.jsx":"c8fd41ff1f7c","components/icons/UnicornSprite.jsx":"4413698c9bfa","components/icons/iconData.js":"4d3e2675177b","components/icons/pixel.js":"0cb93aea7245","components/icons/unicornData.js":"79411f02c8a8","components/rainbow/RainbowBar.jsx":"e7da5339e099","components/rainbow/RainbowFrame.jsx":"7982c4566333","components/rainbow/RainbowText.jsx":"d46b97006fd3","icons/build_icons.js":"37fe9d887c05","icons/generate.js":"5c5e516b8895","ui_kits/device/AppScreen.jsx":"922c41cc5510","ui_kits/device/apps/AppsA.jsx":"32c75cd24442","ui_kits/device/apps/AppsB.jsx":"97ce0e7f292c","ui_kits/device/apps/Frequency.jsx":"03a97e8efe46","ui_kits/device/apps/shared.jsx":"f4c4baa4a39b"},"inlinedExternals":[],"unexposedExports":[{"name":"cellColor","sourcePath":"components/icons/pixel.js"},{"name":"css","sourcePath":"components/icons/pixel.js"},{"name":"cyberHue","sourcePath":"components/icons/pixel.js"},{"name":"drawGrid","sourcePath":"components/icons/pixel.js"},{"name":"hueCss","sourcePath":"components/icons/pixel.js"},{"name":"modeHue","sourcePath":"components/icons/pixel.js"},{"name":"to565","sourcePath":"components/icons/pixel.js"},{"name":"usePhase","sourcePath":"components/icons/pixel.js"},{"name":"useTick","sourcePath":"components/icons/pixel.js"}]} */

(() => {

const __ds_ns = (window.CyberUnicorn5000DesignSystem_f09c19 = window.CyberUnicorn5000DesignSystem_f09c19 || {});

const __ds_scope = {};

(__ds_ns.__errors = __ds_ns.__errors || []);

// components/device/DeviceScreen.jsx
try { (() => {
// The CYD: 2.8" 320x240 ILI9341 on a yellow PCB. `scale` must be an integer.
function DeviceScreen({
  children,
  scale = 2,
  bezel = true,
  label = true,
  style
}) {
  const screen = React.createElement('div', {
    style: {
      width: 320,
      height: 240,
      position: 'relative',
      overflow: 'hidden',
      background: '#000',
      transform: `scale(${scale})`,
      transformOrigin: '0 0'
    }
  }, children);
  const sized = React.createElement('div', {
    style: {
      width: 320 * scale,
      height: 240 * scale,
      imageRendering: 'pixelated'
    }
  }, screen);
  if (!bezel) return React.createElement('div', {
    style
  }, sized);
  return React.createElement('div', {
    style: {
      display: 'inline-flex',
      flexDirection: 'column',
      gap: 8,
      padding: 14,
      paddingBottom: label ? 10 : 14,
      background: '#f2c200',
      borderRadius: 6,
      boxShadow: 'inset 0 0 0 2px #c99a00',
      ...style
    }
  }, React.createElement('div', {
    style: {
      padding: 6,
      background: '#111',
      borderRadius: 2
    }
  }, sized), label && React.createElement('div', {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      fontFamily: 'var(--font-display)',
      fontSize: 7,
      color: '#3a2a00'
    }
  }, React.createElement('span', null, 'ESP32-2432S028'), React.createElement('span', null, 'CU5K')));
}
Object.assign(__ds_scope, { DeviceScreen });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/device/DeviceScreen.jsx", error: String((e && e.message) || e) }); }

// components/device/ScreenText.jsx
try { (() => {
// tft.drawString(): GFX 6x8 font at size 1-4, positioned in device pixels.
function ScreenText({
  x = 0,
  y = 0,
  size = 1,
  color = '#fff',
  align = 'left',
  width,
  bg,
  children,
  style
}) {
  const fs = 6 * size;
  const pos = align === 'center' ? {
    left: width ? x : 0,
    width: width || 320,
    textAlign: 'center'
  } : align === 'right' ? {
    right: 320 - x,
    textAlign: 'right'
  } : {
    left: x
  };
  return React.createElement('div', {
    style: {
      position: 'absolute',
      top: y,
      ...pos,
      fontFamily: "'Press Start 2P', monospace",
      fontSize: fs,
      lineHeight: `${8 * size}px`,
      height: 8 * size,
      paddingTop: size,
      boxSizing: 'border-box',
      color,
      background: bg,
      whiteSpace: 'pre',
      letterSpacing: 0,
      ...style
    }
  }, children);
}
Object.assign(__ds_scope, { ScreenText });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/device/ScreenText.jsx", error: String((e && e.message) || e) }); }

// components/icons/iconData.js
try { (() => {
// Generated by icons/generate.js from icons/build_icons.js. Do not edit.
const ICON_W = 32;
const ICON_H = 24;
const PALETTE = [{
  "k": ".",
  "v": null,
  "d": "transparent"
}, {
  "k": "R",
  "v": "rainbow",
  "d": "animated rainbow (hue by icon mode + time)"
}, {
  "k": "K",
  "v": [24, 6, 40],
  "d": "outline, deep purple"
}, {
  "k": "W",
  "v": [255, 255, 255],
  "d": "body white"
}, {
  "k": "w",
  "v": [200, 180, 255],
  "d": "body shade, lavender"
}, {
  "k": "H",
  "v": [255, 220, 60],
  "d": "horn / sun yellow"
}, {
  "k": "h",
  "v": [255, 140, 0],
  "d": "horn shade, orange"
}, {
  "k": "E",
  "v": [0, 255, 255],
  "d": "cyber eye, neon cyan"
}, {
  "k": "P",
  "v": [255, 90, 200],
  "d": "hot pink"
}, {
  "k": "G",
  "v": [0, 255, 160],
  "d": "neon green (hooves)"
}, {
  "k": "M",
  "v": [255, 0, 200],
  "d": "magenta (grid rays, title shadow)"
}, {
  "k": "B",
  "v": [0, 160, 255],
  "d": "grid blue"
}, {
  "k": "D",
  "v": [30, 0, 70],
  "d": "sky top / panel"
}, {
  "k": "V",
  "v": [96, 0, 48],
  "d": "sky low / dim LED"
}, {
  "k": "N",
  "v": [12, 0, 28],
  "d": "grid floor / screen"
}, {
  "k": "o",
  "v": [255, 150, 120],
  "d": "sun mid"
}, {
  "k": "S",
  "v": "sparkle",
  "d": "twinkle: white, cyan, pink, off"
}, {
  "k": "A",
  "v": "frameA",
  "d": "rainbow, frame A only"
}, {
  "k": "a",
  "v": "frameB",
  "d": "rainbow, frame B only"
}, {
  "k": "Q",
  "v": "gridA",
  "d": "grid blue, frame A only"
}, {
  "k": "q",
  "v": "gridB",
  "d": "grid blue, frame B only"
}];
const ICONS = {
  frequency: {
    id: 'frequency',
    name: "Frequency Check-in",
    mode: 'angle',
    note: "the Frequency FR logo: rainbow ring, white FR on a night-sky disc, striped sun rising",
    grid: ['............RRRRRRRR............', '..........RRRRRRRRRRRR..........', '..S......RRRKKKKKKKKRRR.........', '.......RRRKKDDDDDDDDKKRRR....S..', '.......RRKSDDDDDDDDDDDKRR.......', '...A..RRKDDDDDDDDDDDDDDKRR......', '..AAARRKDWWWWWWWWWWWDDDDKRR.....', '...A.RRKWWWWWWWWWWWWWDDDKRR.....', '....RRKVWWVVVVVVVVVWWWVVVKRR....', '....RRKVWWVVVVVVVVVVWWVVSKRR....', '....RRKSWWWWWVWWWWWWWWVVVKRR....', '....RRKVWWWWWVWWWWWWWVVVVKRR....', '....RRKVWWVVVVWWVVWWVVVVVKRR....', '....RRKVWWVVVVWWVVVWWVVVVKRR....', '....RRKNWWNNNNWWHHNWWNNNNKRR....', '....RRKNwwNNHHwwHHHHwwNNNKRRa...', '.....RRKwwNhhhwwhhhhhwwNKRRaaa..', '.....RRKNNNNNNNNNNNNNNNNKRR.a...', '......RRKooooooooooooooKRR......', '.......RRKNNNNNNNNNNNNKRR.......', '.......RRRKKPPPPPPPPKKRRR.......', '..S......RRRKKKKKKKKRRR......S..', '..........RRRRRRRRRRRR..........', '............RRRRRRRR............']
  },
  lightshow: {
    id: 'lightshow',
    name: "LED Light Show",
    mode: 'diag',
    note: "8x8 LED panel; rainbow heart beats",
    grid: ['...KDDDDDDDDDDDDDDDDDDDDDDDDK...', '..KDDNNDAADAADNNDNNDAADAADNNDK..', '..KDDNNDAADAADNNDNNDAADAADNNDK..', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '..KDDAADRRDRRDAADAADRRDRRDAADK..', '..KDDAADRRDRRDAADAADRRDRRDAADK..', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '..KDDAADRRDRRDRRDRRDRRDRRDAADK..', '..KDDAADRRDRRDRRDRRDRRDRRDAADK..', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '..KDDNNDAADRRDRRDRRDRRDAADNNDK..', '..KDDNNDAADRRDRRDRRDRRDAADNNDK..', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '..KDDNNDNNDAADRRDRRDAADNNDNNDK..', '..KDDNNDNNDAADRRDRRDAADNNDNNDK..', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '..KDDNNDNNDNNDAADAADNNDNNDNNDK..', '..KDDNNDNNDNNDAADAADNNDNNDNNDK..', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '..KDDSNDNNDNNDNNDNNDNNDNNDSNDK..', '..KDDNNDNNDNNDNNDNNDNNDNNDNNDK..', '...KDDDDDDDDDDDDDDDDDDDDDDDDK...', '....KKKKKKKKKKKKKKKKKKKKKKKK....', '..............wwww..............']
  },
  sensors: {
    id: 'sensors',
    name: "Sensor Hub",
    mode: 'col',
    note: "little screen with a rainbow sparkline and node dots",
    grid: ['...KKKKKKKKKKKKKKKKKKKKKKKKKK...', '..KwwwwwwwwwwwwwwwwwwwwwwwwwwK..', '.KwwwwwwwwwwwwwwwwwwwwwwwwwwwwK.', 'KwwNNNNNDNNNNDNNNNDNNNNDNNNNNwwK', 'KwwNNNNNDNNNNDNNNNDNNNNDNNNNNwwK', 'KwwNNNNNDNNNNDNNNNDRNNNDNAANNwwK', 'KwwNNNNNDNNNNDNNNNRRRNNDNAANNwwK', 'KwwDDDDDDDDDDRDDDDRDRRDRRAADDwwK', 'KwwNNNNNDNNNNRRNNRRNNRRRRNNNNwwK', 'KwwNNNNNDNNNRRRRNRDNNNRDNNNNNwwK', 'KwwNNNNNDNNNRDNRRRDNNNNDNNNNNwwK', 'KwwDDDDRDDDRRDDDRDDDDDDDDDDDDwwK', 'KwwNNNRRRRNRNDNNNNDNNNNDNNNNNwwK', 'KwwNNNRNRRRRNDNNNNDNNNNDNNNNNwwK', 'KwwNNRRNDNRNNDNNNNDNNNNDNNNNNwwK', 'KwwDRRDDDDDDDDDDDDDDDDDDDGDGDwwK', 'KwwNRNNNDNNNNDNNNNDNNNNDNNNNNwwK', 'KwwNNNNNDNNNNDNNNNDNNNNDNaNaNwwK', 'KwwNNNNNDNNNNDNNNNDNNNNDNNNNNwwK', '.KwwwwwwwwwwwwwwwwwwwwwwwwwwwwK.', '..KwwwwwwwwwwwwwwwwwwwwwwwwwwK..', '...KKKKKKKKKwwwwwwwwKKKKKKKKK...', '...........KwwwwwwwwK...........', '............KKKKKKKK............']
  },
  led: {
    id: 'led',
    name: "RGB LED",
    mode: 'row',
    note: "5 mm LED: rainbow dome, glow rays",
    grid: ['................................', '........a...............a.......', '.........a.....KKK.....a........', '.............KKRRRKK............', '............KWRRRRRRK...........', '...........KRWRRRRRRRK..........', '..........KRWRRRRRRRRRK.........', '..........KRWRRRRRRRRRK.........', '.........KRRWRRRRRRRRRRK........', '....AAA..KRRWRRRRRRRRRRK..AAA...', '.........KRRWRRRRRRRRRRK........', '.........KRRWRRRRRRRRRRK........', '.........KRRRRRRRRRRRRRK........', '.........KRRRRRRRRRRRRRK........', '.........KRRRRRRRRRRRRRK........', '........KwwwwwwwwwwwwwwwK.......', '........KwwwwwwwwwwwwwwwK.......', '.........KKKKwKKKKKwKKKK........', '............KwK...KwK.......S...', '............KwK...KwK...........', '...S........KwK...KwK...........', '............KwK...KwwK..........', '............KwK....KK...........', '............KwK.................']
  },
  light: {
    id: 'light',
    name: "Light Meter",
    mode: 'angle',
    note: "the boot screen's striped synthwave sun; rays spin",
    grid: ['................a...............', '.........A......a......A........', '..........A...........A.........', '..............KKKKK.............', '............KKHHHHHKK...........', '.....a.....KHHHHHHHHHK.....a....', '......a...KHHHHHHHHHHHK...a.....', '.........KhhhhhhhhhhhhhK........', '........KhhhhhhhhhhhhhhhK.......', '........KhhhhhhhhhhhhhhhK.......', '.......KhhhhhhhhhhhhhhhhhK......', '.......KoooooooooooooooooK......', '...AAA.KoooooooooooooooooK.AAA..', '........KKKKKKKKKKKKKKKKK.......', '.......KoooooooooooooooooK......', '........KPPPPPPPPPPPPPPPK.......', '.........KKKKKKKKKKKKKKK........', '.........KPPPPPPPPPPPPPK........', '......a...KPPPPPPPPPPPK...a.....', '.....a.....KKKKKKKKKKK.....a....', '.............KPPPPPK............', '..............KKKKK.............', '..........AA..........A.........', '..........A.....a......A........']
  },
  sound: {
    id: 'sound',
    name: "Sound & Tones",
    mode: 'row',
    note: "speaker cone, rainbow waves pulse out",
    grid: ['................................', '................................', '..............K.......aa.....S..', '.............KwK.......aa.......', '............KWwK.......aaa......', '...........KWWwK....AA..aa......', '..........KWWWwK....AA...aa.....', '.........KWWWWwK.....AA..aa.....', '...KKKKKKWWWWWwK..R..AA..aaa....', '..KwwwwwWwWWWWwK.RR...AA..aa....', '..KwwwwwWwWWWWwK..RR..AA..aa....', '..KwwwwwWwWWWWwK..RR..AA..aa....', '..KwwwwwWwWWWWwK..RR..AA..aa....', '..KwwwwwWwWWWWwK..RR..AA..aa....', '..KwwwwwWwWWWWwK.RR...AA..aa....', '...KKKKKKWWWWWwK..R..AA..aaa....', '.........KWWWWwK.....AA..aa.....', '..........KWWWwK....AA...aa.....', '...........KWWwK....AA..aa......', '............KWwK.......aaa......', '.............KwK.......aa.......', '..............K.......aa......S.', '................................', '................................']
  },
  slideshow: {
    id: 'slideshow',
    name: "Photo Slideshow",
    mode: 'angle',
    note: "rainbow picture frame around a tiny boot scene",
    grid: ['RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR', 'RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR', 'RRDDDDDDDDDDDDDDDDDDDDDDDDDDDDRR', 'RRDDDDDDDDSDDDDDDDDDDDDDDDDDDDRR', 'RRDDDSDDDDDDDDDDDDDDDDDDDDDSDDRR', 'RRDDDDDDDDDDDDDDDDDDDDDDDDDDDDRR', 'RRDDDDDDDDDDDDSDDDDDDDDDDDDDDDRR', 'RRDDDDDDDDDDDDDDDDDDHHHDDDDDDDRR', 'RRVVVVVVVVVVVVVVVVHHHHHHHVVVVVRR', 'RRVVVVVVVVVVVVVVVhhhhhhhhhVVVVRR', 'RRVVVVVVVVVVVVVVVVVVVVVVVVVVVVRR', 'RRVVVVVVVVVVVVVVPPPPPPPPPPPVVVRR', 'RRVVVVVVVVVVVVVVVVVVVVVVVVVVVVRR', 'RRMMMMMMMMMMMMMMMMMMMMMMMMMMMMRR', 'RRNNNNNNNNMMMMNMMNMMMMMNNNNNNNRR', 'RRBBBBBBBBBBBBBBBBBBBBBBBBBBBBRR', 'RRNNNNMMMMNMNNMNMNNMNMNMMMMNNNRR', 'RRBBBBBBBBBBBBBBBBBBBBBBBBBBBBRR', 'RRMMNMMNNMNNNMNNMNNNMNNMNNMMNMMR', 'MMNNMNNNMNNNMNNNMNNNMNNNMNNNMNRM', 'RRBBBBBBBBBBBBBBBBBBBBBBBBBBBBMR', 'RMNNNNMNNNNMNNNNMNNNNMNNNNMNNNRM', 'RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR', 'RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR']
  },
  weather: {
    id: 'weather',
    name: "Weather Forecast",
    mode: 'col',
    note: "sun behind a cloud, rainbow rain falls",
    grid: ['...........................a....', '....................KKK.........', '..................KKHHHKK.......', '.................KHHHHHHHK...A..', '................KHHHHHHHHHK.....', '................KHHHHHHHHHK.....', '..............KKWWWHHHHHHHHK....', '.............KWWWWWWWHHHHHHK....', '..........KKKWWWWWWWWWhhhhhK..A.', '........KKWWWWWWWWWWWWhhhhK.....', '.......KWWWWWWWWWWWWWWWhhhK.....', '.......KWWWWWWWWWWWWWWWWWK......', '......KWWWWWWWWWWWWWWWWWWWK.....', '......KWWWWWWWWWWWWWWWWWWWWK....', '......KWWWWWWWWWWWWWWWWWWWWK....', '.......KWWWWWWWWWWWWWWWWWWwK....', '.......KWWwwwwWWWWWWWWWWWWK.....', '.......KwwwwwwwwwwwwwwwwwwK.....', '........KKKKKKKKKKKKKKKKKK......', '.........A.....A.....A..........', '.........A..a..A..a..A..a.......', '........a...a.a...a.a...a.......', '........a..A..a..A..a..A........', '...........A.....A.....A........']
  },
  hat: {
    id: 'hat',
    name: "Hat Module Probe",
    mode: 'row',
    note: "the NM-RF-HAT with its DIP switch; DIP 3 (PN532) flips",
    grid: ['................................', '................................', '...K.K.K.K.K.K.K.K.K.K.K.K.K....', '..KhKhKhKhKhKhKhKhKhKhKhKhKhK...', '..KHKHKHKHKHKHKHKHKHKHKHKHKHKK..', '.KDDDDDDDDDDDDDDDDDDDDDDDDDDDDK.', 'KDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDK', 'KDDVDDDDDDDDDDDDDDDDDDDDDDDDVDDK', 'KDDVDDMMMMMMMMMMMMMMMMMMMMDDVDDK', 'KDDVDDMMNNMNNMAAMNNMNNMNNMDDVDDK', 'KDDVDDMMNNMNNMAAMNNMNNMNNMDDVDDK', 'KDDVDDMMNNMNNMAAMNNMNNMNNMDDVDDK', 'KDDVDDMMWWMWWMaaMWWMWWMWWMDDVDDK', 'KDDVDDMMWWMWWMaaMWWMWWMWWMDDVDDK', 'KDDVDDMMWWMWWMaaMWWMWWMWWMDDVDDK', 'KDDVDDMMMMMMMMMMMMMMMMMMMMDDVDDK', 'KDDVDDMMWMMWMMWMMWMMWMMWMMDDVDDK', 'KDDVDDDDDDDDDDDDDDDDDDDDDDDDVDDK', 'KDDVVVVVVVVVVVVVVVVVVVVVVVVVVDDK', '.KDDDDDDDDDDDDDDDDDDDDDDDDDDDDK.', '..KKKKKKKKKKKKKKKKKKKKKKKKKKKK..', '................................', '................................', '................................']
  },
  gpio: {
    id: 'gpio',
    name: "GPIO Pins",
    mode: 'row',
    note: "ESP32 chip with rainbow legs; signals hop pin to pin",
    grid: ['................................', '.........KKKKKKKKKKKKKK.........', '...KKKKKKwwwwwwwwwwwwwwKKKKKK...', '.AARRRRRRwVVVVVVVVVVVVwRRRRRRaa.', '.AARRRRRRwDPPDDDDDDDDDwRRRRRRaa.', '...KKKKKKwDPPDDDDDDDDDwKKKKKK...', '...KKKKKKwDDDDDDDDDDDDwKKKKKK...', '.aaRRRRRRwDDDDDDDDDDDDwRRRRRRAA.', '.aaRRRRRRwDDDDDDDDDDDDwRRRRRRAA.', '...KKKKKKwDDDDDDDDDDDDwKKKKKK...', '...KKKKKKwDDEEEDEEEDDDwKKKKKK...', '.AARRRRRRwDDDDEDDDEDDDwRRRRRRaa.', '.AARRRRRRwDDEEEDEEEDDDwRRRRRRaa.', '...KKKKKKwDDDDEDEDDDDDwKKKKKK...', '...KKKKKKwDDEEEDEEEDDDwKKKKKK...', '.aaRRRRRRwDDDDDDDDDDDDwRRRRRRAA.', '.aaRRRRRRwDDDDDDDDDDDDwRRRRRRAA.', '...KKKKKKwDDDDDDDDDDDDwKKKKKK...', '...KKKKKKwDDDDDDDDDDDDwKKKKKK...', '.AARRRRRRwDDDDDDDDDDDDwRRRRRRaa.', '.AARRRRRRwDDDDDDDDDDDDwRRRRRRaa.', '...KKKKKKwwwwwwwwwwwwwwKKKKKK...', '.........KKKKKKKKKKKKKK.........', '................................']
  },
  remote: {
    id: 'remote',
    name: "Phone Remote",
    mode: 'row',
    note: "phone with a rainbow screen; signal bars fill",
    grid: ['....KKKKKKKK....................', '...KwwwwwwwwK...................', '..KwwwDDDDwwwK...............S..', '.KwwwwwwwwwwwwK.......S.........', '.KwwNNNNNNNNwwK.................', '.KwwNNNNNNNNwwK.................', '.KwwRRRRRRRRwwK...........aa....', '.KwwRRRRRRRRwwK...........aa....', '.KwwRRRRRRRRwwK........KK.aa....', '.KwwRRRRRRRRwwK.......KRRKaa....', '.KwwRRRRRRRRwwK.......KRRKaa....', '.KwwRRRRRRRRwwK.....KKKRRKaa....', '.KwwRRRRRRRRwwK....KRRKRRKaa....', '.KwwRRRRRRRRwwK....KRRKRRKaa....', '.KwwRRRRRRRRwwK..KKKRRKRRKaa....', '.KwwRRRRRRRRwwK.KRRKRRKRRKaa....', '.KwwNNNNNNNNwwK.KRRKRRKRRKaa....', '.KwwNNNNNNNNwwK.KRRKRRKRRKaa....', '.KwwwwwwwwwwwwK.KRRKRRKRRKaa....', '.KwwwwwDDwwwwwK..KK.KK.KK.......', '.KwwwwwDDwwwwwK.................', '..KwwwwwwwwwwK..................', '...KwwwwwwwwK...................', '....KKKKKKKK....................']
  },
  wifi: {
    id: 'wifi',
    name: "WiFi Tools",
    mode: 'row',
    note: "fan of arcs, outer two pulse",
    grid: ['................................', '................................', '................................', '...S........aaaaaaaaa...........', '.........aaaaaaaaaaaaaaa........', '.......aaaaaa.......aaaaaa......', '......aaaa.............aaaa.....', '.....aaa.....AAAAAAA.....aaa....', '....aaa...AAAAAAAAAAAAA...aaa...', '...aaa...AAAA.......AAAA...aaa..', '.......AAAA...........AAAA......', '......AAA....RRRRRRR....AAA.....', '.......A...RRRRRRRRRRR...A......', '..........RRRR.....RRRR.........', '.........RRR.........RRR........', '..............RRRRR.............', '.............RRRRRRR............', '.............RR...RR............', '...............KKK..............', '..............KWWWK.............', '..............KWWWK..........S..', '..............KWWWK.............', '...............KKK..............', '................................']
  },
  ble: {
    id: 'ble',
    name: "Bluetooth LE",
    mode: 'angle',
    note: "blue \"BLE\" badge with broadcast arcs (no Bluetooth rune: trademark)",
    grid: ['................................', '................................', '................................', '.........KKKKKKKKKKKKKK.........', '.......KKBBBBBBBBBBBBBBKK.......', '......KBBBBBBBBBBBBBBBBBBK......', '...a.KBBBBBBBBBBBBBBBBBBBBK..a..', '...a.KABBBBBBBBBBBBBBBAABBA..a..', '...aKBABWWWBBWBBBBWWWBAABBAK.a..', '..a.KABBWBBWBWBBBBWBBBBBBBBA..a.', '..a.KABBWWWBBWBBBBWWBBBBBBBA..a.', '..a.KABBWBBWBWBBBBWBBBBBBBBA..a.', '..a.KABBWBBWBWBBBBWBBBBBBBBA..a.', '..a.KABBWWWBBWWWBBWWWBBBBBBA..a.', '..a.KABBBBBBBBBBBBBBBBBBBBBA..a.', '..a.KABBBBBBBBBBBBBBBBBBBBBA..a.', '...a.KABBBBBBBBBBBBBBBBBBBA..a..', '...a.KABBBBBBBBBBBBBBBBBBBA..a..', '...a..KDDDDDDDDDDDDDDDDDDK...a..', '.......KKBBBBBBBBBBBBBBKK.......', '.........KKKKKKKKKKKKKK.........', '................................', '................................', '................................']
  },
  rf: {
    id: 'rf',
    name: "Sub-GHz Radio",
    mode: 'row',
    note: "Sub-GHz antenna mast radiating both ways",
    grid: ['...aa..AAA............AAA..aa...', '..aaa..AA..............AA..aaa..', '..aa..AAA..R........R..AAA..aa..', '..aa..AA..RRR.KKKK.RRR..AA..aa..', '..aa..AA..RR.KPPPPK.RR..AA..aa..', '..aa..AA..RRKPPPPPPKRR..AA..aa..', '..aa..AA..RRKPPPPPPKRR..AA..aa..', '..aa..AA..RRKPPPPPPKRR..AA..aa..', '..aa..AA..RR.KPPPPK.RR..AA..aa..', '..aa..AA..RRR.KwwK.RRR..AA..aa..', '..aa..AAA..R..KwwK..R..AAA..aa..', '..aaa..AA.....KwwK.....AA..aaa..', '...aa..AAA....KwwK....AAA..aa...', '...aaa..A.....KwwK.....A..aaa...', '....aa........KwwK........aa....', '.....aa.......KwwK.......aa.....', '..............KwwK..............', '..............KwwK..............', '..............KwwK..............', '.............KKwwKK.............', '............KwwwwwwK............', '...........KwwwwwwwwK...........', '..........KwwwwwwwwwwK..........', '...........KKKKKKKKKK...........']
  },
  rfid: {
    id: 'rfid',
    name: "RFID Cards",
    mode: 'col',
    note: "card with a gold chip and holo stripe",
    grid: ['................................', '................................', '................................', '...KKKKKKKKKKKKKKKKKKKKK........', '..KWWWWWWWWWWWWWWWWWWWWWK.......', '.KWWWWWWWWWWWWWWWWWWWWWWWK......', 'KWWWWWWWWWWWWWWWWWWWWWWWWwK.....', 'KWWWWWWWWWWWWWWWWWWWWWWWWwK.a...', 'KWWWHHHhHHHWWWWWWWWWWWWWWwK..a..', 'KWWWHHHhHHHWWwwwwwwwwwwWWwKA.a..', 'KWWWhhhhhhhWWWWWWWWWWWWWWwK.A.a.', 'KWWWHHHhHHHWWWWWWWWWWWWWWwK.A.a.', 'KWWWHHHhHHHWWwwwwwwwWWWWWwK.A.a.', 'KWWWHHHhHHHWWWWWWWWWWWWWWwK.A.a.', 'KWWWWWWWWWWWWWWWWWWWWWWWWwK.A.a.', 'KWWWWWWWWWWWWWWWWWWWWWWWWwKA.a..', 'KWRRRRRRRRRRRRRRRRRRRRRRRwK..a..', 'KWRRRRRRRRRRRRRRRRRRRRRRRwK.a...', 'KWWWWWWWWWWWWWWWWWWWWWWWWwK.....', '.KWWWWWWWWWWWWWWWWWWWWWWWK......', '..KwwwwwwwwwwwwwwwwwwwwwK.......', '...KKKKKKKKKKKKKKKKKKKKK........', '................................', '................................']
  },
  ir: {
    id: 'ir',
    name: "Infrared Remote",
    mode: 'col',
    note: "remote firing a rainbow beam",
    grid: ['................................', '................................', '................................', '................................', '................................', '................................', '................................', '...KKKKKKKKKKK..................', '..KwwwwwwwwwwwK.................', '.KwwwwwwwwwwwwwK.............aa.', 'KwwwPPwwEEwwwwwwKK........AA....', 'KwwwPPwwEEwwDDwwMMK....aa.......', 'KwwwwwwwwwwwDDwwMMK.AA.aa.AA.aa.', 'KwwwGGwwHHwwDDwwMMK....aa.......', 'KwwwGGwwHHwwwwwwKK........AA....', '.KwwwwwwwwwwwwwK.............aa.', '..KwwwwwwwwwwwK.................', '...KKKKKKKKKKK..................', '................................', '................................', '................................', '................................', '................................', '................................']
  },
  nrf24: {
    id: 'nrf24',
    name: "2.4 GHz Radio",
    mode: 'row',
    note: "2.4 GHz module with meander antenna",
    grid: ['................................', '.....aaaaa......................', '...aa.....aa....................', '..a..AAAAA..a...................', '.a..A.....A..a..................', '...A..RRR..A....................', '.....R...R......................', '...KKKKKKKKKKKKKKKKKKKKKKKKKK...', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '.KDDDDDDDDDDDDDDDDDDDDDDDDDDDDK.', '.KDDHHHHHHHDDDDDDDDDDDDDHDHDDDK.', '.KDDDDDDDDHDDDNNNNNNNDDDDDDDDDK.', '.KDDDDDDDDHDDDNENNNNNDDDHDHDDDK.', '.KDDHHHHHHHDDDNNNNNNNDDDDDDDDDK.', '.KDDHDDDDDDDDDNNNNNNNDDDHDHDDDK.', '.KDDHDDDDDDDDDNNNNNNNDDDDDDDDDK.', '.KDDHHHHHHHDDDNNNNNNNDDDHDHDDDK.', '.KDDDDDDDDHDDDNNNNNNNDDDDDDDDDK.', '.KDDDDDDDDHDDDDDDDDDDDDDHDHDDDK.', '.KDDHHHHHHHDDDwwwwDDDDDDDDDDDDK.', '.KDDDDDDDDDDDDDDDDDDDDDDDDDDDDK.', '..KDDDDDDDDDDDDDDDDDDDDDDDDDDK..', '...KKKKKKKKKKKKKKKKKKKKKKKKKK...', '................................']
  },
  gps: {
    id: 'gps',
    name: "GPS & Wardriving",
    mode: 'col',
    note: "map pin landing on the synthwave grid",
    grid: ['...............KKK..............', '.............KKPPPKK............', '............KPPPPPPPK...........', '...........KPPPPPPPPPK..........', '..........KPwPPPPPPPPPK.........', '..........KwPPPWWWPPPPK.........', '.........KPwPPWWWWWPPPPK........', '.........KPPPPWWWWWPPPPK........', '.........KPPPPWWWWWPPPPK........', '..........KPPPPWWWPPPPK.........', '..........KPPPPPPPPPPPK.........', '...........KPPPPPPPPPK..........', '...........KPPPPPPPPPK..........', '............KPPPPPPPK...........', '.....MM.M.M.MPPPPPPPM.M.M.MM....', 'BBBBBBBBBBBBBBPPPPPBBBBBBBBBBBBB', '.MM.MM.MM.M..MPPPPPM..M.MM.MM.MM', 'BBBBBBBBBBBaaaaPPPaaaaBBBBBBBBBB', '.M...M..aaa.AAAPPPAAA.aaa..M...M', 'M..MM.aaM.AAA..KPK..AAA.Maa.MM..', 'BBBBBBaBBBABBBBBBBBBBBABBBaBBBBB', '.M....aa..AAA...M...AAA..aa....M', 'M....M.aaaa.AAAAAAAAA.aaaa.M....', 'BBBBBBBBBBBaaaaaaaaaaaBBBBBBBBBB']
  },
  scripts: {
    id: 'scripts',
    name: "JS Scripts",
    mode: 'col',
    note: "terminal with rainbow title bar; cursor blinks",
    grid: ['................................', '.KKKKKKKKKKKKKKKKKKKKKKKKKKKKKK.', 'KwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwK', 'KwRRRRRRRRRRRRRRRRRRRRRRRRRRRRwK', 'KwRRRRRRRRRRRRRRRRRRRRRRRRRRRRwK', 'KwNNNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNGNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNGNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNNGNNEEEEEEENwwwwwwwNNNNNNNwK', 'KwNNGNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNGNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNPPPPPPNwwwwwwwwwwNNNNNNNNNwK', 'KwNNNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNNNNNNNNNAANNNNNNNNNNNNNNNNwK', 'KwNNHHHHHHHNAANNNNNNNNNNNNNNNNwK', 'KwNNNNNNNNNNAANNNNNNNNNNNNNNNNwK', 'KwNNNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwNNNNNNNNNNNNNNNNNNNNNNNNNNNNwK', 'KwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwK', '.KKKKKKKKKKKKKKKKKKKKKKKKKKKKKK.', '................................']
  },
  others: {
    id: 'others',
    name: "More Tools",
    mode: 'angle',
    note: "four-point rainbow sparkle",
    grid: ['................K...............', '...............KRK..............', '..........S....KRK..............', '.....A.........KRK........A.....', '....AAA........KRK.......AAA....', '.....A.........KRK........A.....', '...............KRK..............', '..............KRRRK.............', '..............KRRRK.............', '.............KRRRRRK............', '...........KKRRRRRRRKK..........', '.....KKKKKKRRRRRWRRRRRKKKKKK....', '....KRRRRRRRRRRWWRRRRRRRRRRRK...', '.....KKKKKKRRRRRRRRRRRKKKKKK....', '...........KKRRRRRRRKK..........', '.............KRRRRRK............', '..............KRRRK.............', '..............KRRRK.............', '.....a.........KRK.........a....', '....aaa........KRK........aaa...', '.....a.........KRK.........a....', '...............KRK.....S........', '...............KRK..............', '...............KRK..............']
  },
  clock: {
    id: 'clock',
    name: "Desk Clock",
    mode: 'angle',
    note: "rainbow-rimmed clock; second hand ticks",
    grid: ['................................', '..............KKKKK.............', '...........KKKRRRRRKKK..........', '..........KRRRRRRRRRRRK.........', '.........KRRRRWWWWWRRRRK........', '........KRRRWWWWPWWWWRRRK.......', '.......KRRRWWWWWWWWWWWRRRK......', '......KRRRWWWWWWKWWWWWWRRRK.....', '......KRRWWWAWWWKWWWWWWWRRK.....', '......KRRWWWWAWWKWWWWaWWRRK.....', '.....KRRWWWWWWAWKWWaaWWWWRRK....', '.....KRRWWWWWWWAKaaWWWWWWRRK....', '.....KRRWPWWWWWWPWWWWWWPWRRK....', '.....KRRWWWWWWWWWKKWWWWWWRRK....', '.....KRRWWWWWWWWWWWKKWWWWRRK....', '......KRRWWWWWWWWWWWWWWWRRK.....', '......KRRWWWWWWWWWWWWWWWRRK.....', '......KRRRWWWWWWWWWWWWWRRRK.....', '.......KRRRWWWWWWWWWWWRRRK......', '........KRRRWWWWPWWWWRRRK.......', '.........KRRRRWWWWWRRRRK........', '..........KRRRRRRRRRRRK.........', '...........KKKRRRRRKKK..........', '..............KKKKK.............']
  },
  connect: {
    id: 'connect',
    name: "File Share",
    mode: 'row',
    note: "two Cheap Yellow Displays swapping a file",
    grid: ['................................', '................................', '...S............................', '................................', '................................', '.KKKKKKKKKKK........KKKKKKKKKKK.', 'KHHHHHHHHHHHK......KHHHHHHHHHHHK', 'KHNNNNNNNNNHK......KHNNNNNNNNNHK', 'KHNNNNNNNNNHK...A..KHNNWWWWNNNHK', 'KHNRRRRRRRNHKAAAAA.KHNNWWWWNNNHK', 'KHNRRRRRRRNHK...A..KHNNWwwWNNNHK', 'KHNRRRRRRRNHK......KHNNWwwWNNNHK', 'KHNRRRRRRRNHK..a...KHNNWWWWNNNHK', 'KHNNNNNNNNNHK.aaaaaKHNNWWWWNNNHK', 'KHNNNNNNNNNHK..a...KHNNNNNNNNNHK', 'KHHHHHHHHHHHK......KHHHHHHHHHHHK', 'KhhhhhhhhhhhK......KhhhhhhhhhhhK', '.KKKKKKKKKKK........KKKKKKKKKKK.', '................................', '................................', '................................', '............................S...', '................................', '................................']
  },
  config: {
    id: 'config',
    name: "Settings",
    mode: 'angle',
    note: "gear with a rainbow hub; sparks orbit",
    grid: ['................................', '...........a...KKK...A..........', '..............KwwwK.............', '.........K....KwwwK...KK........', '........KwK...KwwwK..KwwK.......', '.......KwwwKKKwwwwwKKwwwwK......', '.......KwwwwwwwwwwwwwwwwK.......', '.....A..KwwwwWRRRRRwwwwK...a....', '.........KwwWRRRRRRRwwwK........', '.........KwWRRRwwwRRRwwK........', '......KKKwwRRRwKKKwRRRwwKKK.....', '.....KwwwwwRRwK...KwRRwwwwwK....', '.....KwwwwwRRwK...KwRRwwwwwK....', '.....KwwwwwRRwK...KwRRwwwwwK....', '......KKKwwRRRwKKKwRRRwwKKK.....', '.........KwwRRRwwwRRRwwK........', '.........KwwwRRRRRRRwwwK........', '.....a...KwwwwRRRRRwwwwwK..A....', '........KwwwwwwwwwwwwwwwwK......', '.......KwwwwKKwwwwwKKKwwwK......', '........KwwK..KwwwK...KwK.......', '.........KK...KwwwK....K........', '..............KwwwK.............', '...........A...KKK...a..........']
  }
};
const ICON_ORDER = ["frequency", "lightshow", "sensors", "led", "light", "sound", "slideshow", "weather", "hat", "gpio", "remote", "wifi", "ble", "rf", "rfid", "ir", "nrf24", "gps", "scripts", "others", "clock", "connect", "config"];
Object.assign(__ds_scope, { ICON_W, ICON_H, PALETTE, ICONS, ICON_ORDER });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/icons/iconData.js", error: String((e && e.message) || e) }); }

// components/icons/pixel.js
try { (() => {
// Shared pixel engine: the firmware's colour maths, ported 1:1.

// cyberHue(): full-saturation hue (deg) -> [r,g,b]. Same sector maths as rainbow.cpp.
function cyberHue(hue) {
  let h = hue % 360;
  if (h < 0) h += 360;
  const s = Math.floor(h / 60),
    f = Math.floor(h % 60 * 255 / 60);
  switch (s) {
    case 0:
      return [255, f, 0];
    case 1:
      return [255 - f, 255, 0];
    case 2:
      return [0, 255, f];
    case 3:
      return [0, 255 - f, 255];
    case 4:
      return [f, 0, 255];
    default:
      return [255, 0, 255 - f];
  }
}
// Quantise to RGB565 and back, so the web shows what the panel shows.
function to565([r, g, b]) {
  return [r & 0xf8 | r >> 5, g & 0xfc | g >> 6, b & 0xf8 | b >> 5];
}
const css = ([r, g, b]) => `rgb(${r},${g},${b})`;
function modeHue(mode, x, y, w = 32, h = 24) {
  if (mode === 'row') return y * 18;
  if (mode === 'col') return x * 12;
  if (mode === 'diag') return (x + y) * 10;
  return Math.atan2(y - (h - 1) / 2, x - (w - 1) / 2) * 180 / Math.PI + 180;
}
const LUT = Object.fromEntries(__ds_scope.PALETTE.map(p => [p.k, p.v]));

// Colour of one cell at tick t, or null for nothing drawn.
function cellColor(k, x, y, mode, t, opts = {}) {
  const v = LUT[k];
  if (v === null || v === undefined) return null;
  const frame = opts.frame ?? Math.floor(t / 6) % 2;
  const hueOf = () => cyberHue(modeHue(mode, x, y) + t * 12);
  if (v === 'rainbow') return hueOf();
  if (v === 'frameA') return frame === 0 ? hueOf() : null;
  if (v === 'frameB') return frame === 1 ? hueOf() : null;
  if (v === 'gridA') return frame === 0 ? [0, 160, 255] : [12, 0, 28];
  if (v === 'gridB') return frame === 1 ? [0, 160, 255] : [12, 0, 28];
  if (v === 'sparkle') {
    const p = (x * 3 + y * 5 + Math.floor(t / 4)) % 6;
    return p === 0 ? [255, 255, 255] : p === 1 ? [0, 255, 255] : p === 2 ? [255, 90, 200] : null;
  }
  return v;
}
function drawGrid(ctx, grid, mode, t, cell, ox = 0, oy = 0, opts = {}) {
  for (let y = 0; y < grid.length; y++) {
    const row = grid[y];
    for (let x = 0; x < row.length; x++) {
      const c = cellColor(row[x], x, y, mode, t, opts);
      if (!c) continue;
      ctx.fillStyle = css(opts.raw ? c : to565(c));
      ctx.fillRect(ox + x * cell, oy + y * cell, cell, cell);
    }
  }
}

// cyberPhase(): hue offset drifting 1 degree per 25 ms.
function usePhase(running = true, start = 0) {
  const [p, setP] = React.useState(start);
  React.useEffect(() => {
    if (!running) {
      setP(start);
      return;
    }
    let raf,
      last = 0;
    const loop = now => {
      if (now - last >= 40) {
        last = now;
        setP(Math.floor(now / 25) % 360);
      }
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [running, start]);
  return p;
}
const hueCss = h => css(to565(cyberHue(h)));

// 25 fps tick counter (FRAME_MS = 40). Pass false to freeze at `start`.
function useTick(running = true, start = 0) {
  const [t, setT] = React.useState(start);
  React.useEffect(() => {
    if (!running) {
      setT(start);
      return;
    }
    let raf,
      last = performance.now(),
      acc = 0;
    const loop = now => {
      acc += now - last;
      last = now;
      if (acc >= 40) {
        setT(v => v + Math.floor(acc / 40));
        acc %= 40;
      }
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [running, start]);
  return t;
}
Object.assign(__ds_scope, { cyberHue, to565, css, modeHue, cellColor, drawGrid, usePhase, hueCss, useTick });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/icons/pixel.js", error: String((e && e.message) || e) }); }

// components/icons/PixelIcon.jsx
try { (() => {
function PixelIcon({
  name = 'frequency',
  cell = 3,
  animate = true,
  frame,
  tick = 0,
  background,
  style,
  title
}) {
  const icon = __ds_scope.ICONS[name];
  const ref = React.useRef(null);
  const t = __ds_scope.useTick(animate, tick);
  const w = 32 * cell,
    h = 24 * cell;
  React.useEffect(() => {
    const c = ref.current;
    if (!c || !icon) return;
    const ctx = c.getContext('2d');
    ctx.clearRect(0, 0, w, h);
    if (background) {
      ctx.fillStyle = background;
      ctx.fillRect(0, 0, w, h);
    }
    __ds_scope.drawGrid(ctx, icon.grid, icon.mode, t, cell, 0, 0, frame === undefined ? {} : {
      frame
    });
  }, [t, icon, cell, frame, background, w, h]);
  if (!icon) return null;
  return React.createElement('canvas', {
    ref,
    width: w,
    height: h,
    role: 'img',
    'aria-label': title || icon.name,
    style: {
      width: w,
      height: h,
      imageRendering: 'pixelated',
      display: 'block',
      ...style
    }
  });
}
Object.assign(__ds_scope, { PixelIcon });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/icons/PixelIcon.jsx", error: String((e && e.message) || e) }); }

// components/icons/unicornData.js
try { (() => {
// Boot unicorn, copied verbatim from cyberpunkunicorn-os/assets/make_unicorn.py.
// 'R' is the animated rainbow mane (hue = row*18 + t*12).
const UNICORN_PALETTE = {
  K: [24, 6, 40],
  W: [255, 255, 255],
  w: [200, 180, 255],
  H: [255, 220, 60],
  h: [255, 140, 0],
  E: [0, 255, 255],
  P: [255, 90, 200],
  G: [0, 255, 160]
};
const UNICORN_FRAMES = [['...................................H....', '..................................HhK...', '.................................HhK....', '................................HhK.....', '.......................RRRR....KHhK.....', '.....................RRRRRRRKKKWWK......', '....................RRRRRRRKWWWWWWK.....', '...................RRRRRRRKWWWWWEEWK....', '..................RRRRRRRKWWWWWWEEWWK...', '..................RRRRRRKWWWWWWWWWWWWK..', '.................RRRRRRKWWWWWWWWWWWWPWK.', '.................RRRRRKWWWWWWKKKWWWWWWK.', '................RRRRRKWWWWWWK..KKKKKKK..', '................RRRRKWWWWWWK............', '..RRRR.........RRRRKWWWWWWWK............', '.RRRRRRR...KKKKKKKKWWWWWWWWK............', 'RRRRRRRRKKKWWWWWWWWWWWWWWWWK............', 'RRRRRRRKWWWWWWWWWWWWWWWWWWWWK...........', '.RRRRRKWWWWWWWWWWWWWWWWWWWWWK...........', '..RRRRKWWWWWWWWWWWWWWWWWWWWWK...........', '...RRRKwWWWWWWWWWWWWWWWWWWWwK...........', '....RRKwwWWWWWWWWWWWWWWWWWwwK...........', '.....RKwwwKKwwwwwwwwwwKKwwwK............', '......KwwK.KKKKKKKKKKK.KwwK.............', '.....KwwK...............KwwK............', '....KwwK.................KwwK...........', '...KwwK...................KwwK..........', '..KGGK.....................KGGK.........', '..KKK.......................KKK.........'], ['...................................H....', '..................................HhK...', '.................................HhK....', '................................HhK.....', '.......................RRRR....KHhK.....', '.....................RRRRRRRKKKWWK......', '....................RRRRRRRKWWWWWWK.....', '...................RRRRRRRKWWWWWEEWK....', '..................RRRRRRRKWWWWWWEEWWK...', '..................RRRRRRKWWWWWWWWWWWWK..', '.................RRRRRRKWWWWWWWWWWWWPWK.', '.................RRRRRKWWWWWWKKKWWWWWWK.', '................RRRRRKWWWWWWK..KKKKKKK..', '................RRRRKWWWWWWK............', '...............RRRRKWWWWWWWK............', 'RRRRRRRR...KKKKKKKKWWWWWWWWK............', '.RRRRRRRKKKWWWWWWWWWWWWWWWWK............', '..RRRRRKWWWWWWWWWWWWWWWWWWWWK...........', '...RRRKWWWWWWWWWWWWWWWWWWWWWK...........', '...RRRKWWWWWWWWWWWWWWWWWWWWWK...........', '..RRRRKwWWWWWWWWWWWWWWWWWWWwK...........', '.RRRR.KwwWWWWWWWWWWWWWWWWWwwK...........', '......KwwwKKwwwwwwwwwwKKwwwK............', '.......KwwKKKKKKKKKKKKKwwK..............', '........KwwK.........KwwK...............', '.........KwwK.......KwwK................', '..........KwwK.....KwwK.................', '..........KGGK.....KGGK.................', '..........KKK......KKK..................']];
Object.assign(__ds_scope, { UNICORN_PALETTE, UNICORN_FRAMES });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/icons/unicornData.js", error: String((e && e.message) || e) }); }

// components/device/BootScene.jsx
try { (() => {
// Canvas port of unicornBootScene() in apps/boot/unicorn_boot.cpp, extended with
// the blast-off finale: gallop, tremble, rocket away on a rainbow jet, flood the screen.
const TITLE_H = 34,
  SCENE_H = 170,
  HORIZON = 110,
  UNI = 3,
  SPARKS = 18;
const LOADING = ['Polishing horn...', 'Reticulating rainbows...', 'Downloading more RAM...', 'Charging glitter capacitors...', 'Bribing the CC1101...', 'Untangling the mane...', 'Installing hooves.dll...', 'Teaching IR to see colour...', 'Negotiating with the nRF24...', 'Overclocking sparkles...', 'Summoning the PN532 spirit...', 'Calibrating vibes...'];
const c565 = (r, g, b) => __ds_scope.css(__ds_scope.to565([r, g, b]));
const hue = h => __ds_scope.css(__ds_scope.to565(__ds_scope.cyberHue(h)));
function drawText(ctx, s, x, y, size, color, right) {
  ctx.font = `${6 * size}px 'Press Start 2P', monospace`;
  ctx.textBaseline = 'top';
  ctx.fillStyle = color;
  const w = s.length * 6 * size;
  let x0 = right ? x - w : x;
  for (let i = 0; i < s.length; i++) ctx.fillText(s[i], x0 + i * 6 * size, y + size);
}
function BootScene({
  title = 'CYBERUNICORN 5000',
  tagline = 'All systems fabulous.',
  duration,
  loop = true,
  scale = 1,
  finale = true,
  onDone,
  style
}) {
  duration = duration ?? (finale ? 10000 : 7000);
  const ref = React.useRef(null);
  React.useEffect(() => {
    const cv = ref.current,
      ctx = cv.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    let rng = 0xC0FFEE42 >>> 0;
    const rnd = () => {
      rng = Math.imul(rng, 1664525) + 1013904223 >>> 0;
      return rng >>> 8;
    };
    let sparks = Array.from({
      length: SPARKS
    }, () => ({
      x: 0,
      y: 0,
      life: 0
    }));
    let t = 0,
      start = performance.now(),
      raf,
      last = 0,
      done = false;
    const titleCv = document.createElement('canvas');
    titleCv.width = 320;
    titleCv.height = TITLE_H;
    const tctx = titleCv.getContext('2d');
    const sky = t => {
      for (let y = 0; y < HORIZON; y++) {
        const k = y * 255 / HORIZON | 0;
        ctx.fillStyle = c565(30 + k / 3 | 0, 0, 70 - k / 6 | 0);
        ctx.fillRect(0, TITLE_H + y, 320, 1);
      }
      ctx.fillStyle = '#fff';
      for (let i = 0; i < 24; i++) {
        const x = (i * 97 + 13) % 320,
          y = (i * 53 + 7) % (HORIZON - 30);
        if ((i * 7 + (t / 6 | 0)) % 9 !== 0) ctx.fillRect(x, TITLE_H + y, 1, 1);
      }
      const cx = 228,
        cy = HORIZON,
        r = 46;
      for (let y = cy - r; y < cy; y++) {
        const dy = cy - y,
          dx = Math.sqrt(r * r - dy * dy) | 0;
        if (dy < 22 && dy % 6 < 2) continue;
        const k = (y - (cy - r)) * 255 / r | 0;
        ctx.fillStyle = c565(255, 220 - (k * 160 / 255 | 0), 60 + (k * 120 / 255 | 0));
        ctx.fillRect(cx - dx, TITLE_H + y, 2 * dx, 1);
      }
    };
    const grid = t => {
      const depth = SCENE_H - HORIZON,
        o = TITLE_H;
      ctx.fillStyle = c565(12, 0, 28);
      ctx.fillRect(0, o + HORIZON, 320, depth);
      const phase = t % 24 / 24;
      for (let i = 0; i < 9; i++) {
        const z = (i + phase) / 9,
          y = HORIZON + (z * z * depth | 0);
        ctx.fillStyle = c565(0, 160 + (z * 95 | 0), 255);
        ctx.fillRect(0, o + y, 320, 1);
      }
      ctx.strokeStyle = c565(255, 0, 200);
      ctx.lineWidth = 1;
      ctx.save();
      ctx.beginPath();
      ctx.rect(0, o + HORIZON, 320, depth);
      ctx.clip();
      for (let i = -11; i <= 11; i++) {
        ctx.beginPath();
        ctx.moveTo(160 + i * 9 + 0.5, o + HORIZON);
        ctx.lineTo(160 + i * 64 + 0.5, o + SCENE_H - 1);
        ctx.stroke();
      }
      ctx.restore();
      ctx.fillStyle = c565(255, 120, 255);
      ctx.fillRect(0, o + HORIZON, 320, 1);
    };
    const trail = (ux, uy, t) => {
      const top = uy + 15 * UNI;
      for (let x = 0; x < ux + 6 * UNI; x++) {
        const wave = 4 * Math.sin((x + t * 6) / 11) | 0;
        for (let b = 0; b < 6; b++) {
          ctx.fillStyle = hue(b * 60 - t * 8);
          ctx.fillRect(x, TITLE_H + top + b * 4 + wave, 1, 4);
        }
      }
    };
    const sparksDraw = (ux, uy, t) => sparks.forEach((k, i) => {
      if (k.life === 0) {
        k.x = rnd() % (ux + 40 > 0 ? ux + 40 : 1);
        k.y = uy + 30 + rnd() % 50 - 25;
        k.life = 6 + rnd() % 10;
      }
      k.x -= 3;
      k.life--;
      const a = k.life > 4 ? 2 : 1;
      ctx.fillStyle = hue(i * 40 + t * 10);
      ctx.fillRect(k.x - a, TITLE_H + k.y, 2 * a + 1, 1);
      ctx.fillRect(k.x, TITLE_H + k.y - a, 1, 2 * a + 1);
    });
    const unicorn = (ux, uy, f, t) => __ds_scope.UNICORN_FRAMES[f].forEach((row, y) => {
      const mane = hue(y * 18 + t * 12);
      for (let x = 0; x < row.length; x++) {
        const k = row[x];
        if (k === '.') continue;
        ctx.fillStyle = k === 'R' ? mane : __ds_scope.css(__ds_scope.to565(__ds_scope.UNICORN_PALETTE[k]));
        ctx.fillRect(ux + x * UNI, TITLE_H + uy + y * UNI, UNI, UNI);
      }
    });
    // Unicorn at any scale, squashed horizontally (sx < 1) as it banks away from us.
    const unicornAt = (cx, top, sc, sx, f, t) => __ds_scope.UNICORN_FRAMES[f].forEach((row, y) => {
      const mane = hue(y * 18 + t * 12),
        w = Math.max(1, Math.ceil(sc * sx)),
        h = Math.max(1, Math.ceil(sc));
      for (let x = 0; x < row.length; x++) {
        const k = row[x];
        if (k === '.') continue;
        ctx.fillStyle = k === 'R' ? mane : __ds_scope.css(__ds_scope.to565(__ds_scope.UNICORN_PALETTE[k]));
        ctx.fillRect(Math.round(cx + (x - 20) * sc * sx), Math.round(TITLE_H + top + y * sc), w, h);
      }
    });
    const titleDraw = t => {
      tctx.fillStyle = '#000';
      tctx.fillRect(0, 0, 320, TITLE_H);
      let size = title.length * 18 > 316 ? 2 : 3;
      const cw = 6 * size,
        w = cw * title.length + 12,
        x0 = (320 - w) / 2 | 0,
        y0 = (TITLE_H - 8 * size) / 2 | 0;
      const glitch = rnd() % 7 === 0,
        jitter = glitch ? rnd() % 9 - 4 : 0;
      drawText(tctx, title, x0 - 2 + jitter, y0, size, c565(0, 255, 255));
      drawText(tctx, title, x0 + 2 - jitter, y0 + (glitch ? 1 : 0), size, c565(255, 0, 160));
      for (let i = 0; i < title.length; i++) drawText(tctx, title[i], x0 + i * cw, y0, size, hue(i * 22 - t * 9));
      drawText(tctx, 'TM', x0 + cw * title.length + 1, y0, 1, '#fff');
      if (glitch) {
        tctx.fillStyle = hue(rnd() % 360);
        tctx.fillRect(0, y0 + rnd() % (8 * size), 320, 2 + rnd() % 3);
      }
    };
    // Phases (fractions of duration): gallop, tremble during the loader's stall,
    // blast-off on a rainbow jet, then the jet floods the whole screen.
    const P_TREMBLE = 0.46,
      P_BLAST = 0.6,
      P_WASH = 0.86;
    const loading = (elapsed, t) => {
      const y = TITLE_H + SCENE_H,
        barX = 16,
        barY = y + 6,
        barW = 288,
        barH = 10;
      ctx.fillStyle = '#000';
      ctx.fillRect(0, y, 320, 240 - y);
      if (!finale) {
        const k0 = elapsed / duration;
        const pct0 = k0 < 0.45 ? k0 / 0.45 * 64 | 0 : k0 < 0.62 ? 64 + ((k0 - 0.45) * 18 | 0) : 67 + ((k0 - 0.62) / 0.38 * 70 | 0);
        const dn0 = pct0 >= 100 ? barW - 2 : (barW - 2) * pct0 / 100 | 0;
        ctx.strokeStyle = c565(0, 255, 255);
        ctx.strokeRect(barX + 0.5, barY + 0.5, barW - 1, barH - 1);
        for (let x = 0; x < dn0; x++) {
          ctx.fillStyle = hue(x * 2 - t * 10);
          ctx.fillRect(barX + 1 + x, barY + 1, 1, barH - 2);
        }
        drawText(ctx, pct0 >= 100 ? tagline : LOADING[(k0 * 1.8 * LOADING.length | 0) % LOADING.length], barX, barY + barH + 6, 1, '#fff');
        drawText(ctx, pct0 >= 100 ? `${pct0}% (oops)` : `${pct0}%`, barX + barW, barY + barH + 6, 1, pct0 >= 100 ? hue(t * 20) : c565(255, 0, 200), true);
        return;
      }
      const k = elapsed / duration;
      const pct = k < 0.4 ? k / 0.4 * 64 | 0 : k < P_BLAST ? 64 + ((k - 0.4) * 14 | 0) : Math.min(137, 67 + ((k - P_BLAST) / (P_WASH - P_BLAST) * 70 | 0));
      const fill = barW - 2,
        dn = pct >= 100 ? fill : fill * pct / 100 | 0;
      ctx.strokeStyle = c565(0, 255, 255);
      ctx.strokeRect(barX + 0.5, barY + 0.5, barW - 1, barH - 1);
      for (let x = 0; x < dn; x++) {
        ctx.fillStyle = hue(x * 2 - t * 10);
        ctx.fillRect(barX + 1 + x, barY + 1, 1, barH - 2);
      }
      let text;
      if (k >= P_BLAST) text = k < P_BLAST + 0.03 ? 'Banking left...' : pct >= 100 ? 'BLAST OFF!!!' : 'Venting rainbows...';else if (k >= P_TREMBLE) text = (t >> 3) % 2 ? 'Uh oh. Rainbow pressure rising...' : 'Something is brewing...';else text = LOADING[(k * 1.8 * LOADING.length | 0) % LOADING.length];
      drawText(ctx, text, barX, barY + barH + 6, 1, k >= P_TREMBLE ? hue(t * 20) : '#fff');
      drawText(ctx, pct >= 100 ? `${pct}% (oops)` : `${pct}%`, barX + barW, barY + barH + 6, 1, pct >= 100 ? hue(t * 20) : c565(255, 0, 200), true);
    };
    // 4-point pixel sparkle.
    const star = (x, y, r, c) => {
      ctx.fillStyle = c;
      ctx.fillRect(x - r, y, 2 * r + 1, 1);
      ctx.fillRect(x, y - r, 1, 2 * r + 1);
      if (r > 2) {
        ctx.fillStyle = '#fff';
        ctx.fillRect(x - 1, y - 1, 3, 3);
      }
    };
    const SPARK_C = ['#ffffff', c565(0, 255, 255), c565(255, 90, 200), c565(255, 220, 60)];
    let plume = [],
      jet = [];
    const snap = v => Math.round(v / 3) * 3;
    const drawPlume = t => {
      // Jet ribbon: six rainbow bands along the rear's path, fattening with age.
      for (let j = 0; j < jet.length; j++) {
        const p = jet[j],
          age = jet.length - j,
          w = Math.min(60, p.sc * 2 * (1 + age * 0.12));
        for (let b = 0; b < 6; b++) {
          ctx.fillStyle = hue(b * 60 - t * 10);
          ctx.fillRect(snap(p.x - w), TITLE_H + snap(p.y + (b - 3) * w * 0.9), snap(w * 2) + 3, Math.ceil(w * 0.9) + 1);
        }
      }
      // Blobs: chunky rainbow pixels that spray back, swell and pile up.
      for (const q of plume) {
        const z = q.s / 3;
        q.x += q.vx * z;
        q.y += q.vy * z;
        if (q.s < q.max) q.s *= q.g;
        ctx.fillStyle = hue(q.h - t * 6);
        const s = snap(q.s) || 3;
        ctx.fillRect(snap(q.x), TITLE_H + snap(q.y), s, s);
      }
      for (const q of plume) if ((q.id + t) % 7 === 0) star(snap(q.x) + 1, TITLE_H + snap(q.y) + 1, 2 + q.id % 3, SPARK_C[(q.id + (t >> 2)) % 4]);
    };
    const wash = (w, t) => {
      // The flood: a diagonal rainbow wipe from bottom-left that eats the screen.
      const front = w * 300 - 30;
      for (let y = 0; y < 240; y += 6) for (let x = 0; x < 320; x += 6) {
        const d = 240 - y;
        if (d > front + Math.sin((x + t * 6) / 16) * 14) continue;
        ctx.fillStyle = hue((x + y) * 1.2 - t * 14);
        ctx.fillRect(x, y, 6, 6);
      }
      for (let i = 0; i < 40; i++) {
        const x = (i * 97 + t * 13) % 320,
          y = (i * 61 + t * 7) % 240;
        if (240 - y < front && (i + (t >> 1)) % 3) star(x, y, 2 + i % 4, SPARK_C[i % 4]);
      }
      if (w >= 1) {
        const msg = tagline,
          sz = msg.length * 12 > 300 ? 1 : 2,
          x0 = (320 - msg.length * 6 * sz) / 2 | 0,
          y0 = 112;
        drawText(ctx, msg, x0 - 2, y0, sz, c565(0, 255, 255));
        drawText(ctx, msg, x0 + 2, y0, sz, c565(255, 0, 160));
        drawText(ctx, msg, x0, y0, sz, '#fff');
      }
    };
    const frame = now => {
      raf = requestAnimationFrame(frame);
      if (now - last < 40) return;
      last = now;
      let elapsed = now - start;
      if (elapsed >= duration) {
        if (!done && onDone) onDone();
        if (!loop) {
          done = true;
          return;
        }
        start = now;
        elapsed = 0;
        t = 0;
        sparks.forEach(s => s.life = 0);
        plume = [];
        jet = [];
      }
      const k = finale ? elapsed / duration : elapsed / duration * P_TREMBLE / 0.999;
      let ux = -110 + Math.min(t * 6, 170),
        uy = 22 + (3 * Math.sin(t / 2) | 0),
        f = (t / 4 | 0) % 2,
        shake = 0;
      if (k >= P_TREMBLE && k < P_BLAST) {
        const n = (k - P_TREMBLE) / (P_BLAST - P_TREMBLE),
          amp = 1 + n * 4;
        ux += (rnd() % 3 - 1) * amp;
        uy = 22 + (rnd() % 3 - 1) * amp;
        f = 0;
        shake = n > 0.6 ? 1 : 0;
      }
      // Blast-off: bank left (squash) to face the horizon, then rocket up and away
      // over the sun, shrinking into the distance, spraying rainbow at the viewer.
      let far = null;
      if (k >= P_BLAST) {
        const n = Math.min(1, (k - P_BLAST) / (P_WASH - P_BLAST));
        let cx = 120,
          top = 22,
          sc = 3,
          sx = 1;
        if (n < 0.16) {
          const tn = n / 0.16;
          sx = 1 - 0.55 * tn;
          top = 22 - tn * 6;
          shake = 2;
          f = 0;
        } else {
          const m = (n - 0.16) / 0.84,
            e = 1 - (1 - m) * (1 - m);
          sx = 0.45;
          sc = 3 - 2.55 * e;
          cx = 120 + (226 - 120) * e;
          top = 16 + (26 - 16) * e - Math.sin(m * Math.PI) * 18 - sc * 6;
          f = (t / 2 | 0) % 2;
          shake = sc > 1.5 ? 3 : 1;
        }
        far = {
          cx,
          top,
          sc,
          sx,
          n
        };
        const rx = cx,
          ry = top + 18 * sc;
        if (n >= 0.1) {
          jet.push({
            x: rx,
            y: ry,
            sc
          });
          if (jet.length > 70) jet.shift();
          const burst = 14 + (n * 26 | 0);
          for (let i = 0; i < burst && plume.length < 1400; i++) plume.push({
            id: plume.length,
            x: rx + (rnd() % 5 - 2) * sc,
            y: ry,
            vx: (rnd() % 13 - 6) * 0.45,
            vy: (rnd() % 11 - 2) * 0.45,
            s: Math.max(1.5, sc),
            g: 1.05 + rnd() % 5 * 0.01,
            max: 24 + rnd() % 24,
            h: plume.length * 11 % 360
          });
        }
      }
      if (shake) ctx.setTransform(1, 0, 0, 1, rnd() % (2 * shake + 1) - shake, rnd() % (2 * shake + 1) - shake);
      sky(t);
      grid(t);
      if (k < P_BLAST) trail(ux, uy, t);
      if (k < P_TREMBLE) sparksDraw(ux, uy, t);
      if (far) {
        if (far.n < 0.97) unicornAt(far.cx, far.top, far.sc, far.sx, f, t);else star(far.cx | 0, TITLE_H + (far.top | 0) + 6, 6, SPARK_C[t % 4]);
      } else unicorn(ux | 0, uy | 0, f, t);
      if (k >= P_BLAST) drawPlume(t);
      if (k >= P_TREMBLE && k < P_BLAST && (t >> 2) % 2) {
        drawText(ctx, '!!', ux + 34 * UNI, TITLE_H + uy - 18, 2, c565(255, 0, 200));
        ctx.fillStyle = c565(0, 255, 255);
        ctx.fillRect(ux + 26 * UNI + t % 3, TITLE_H + uy + 4 + t % 6 * 2, 2, 3);
      }
      if (t % 3 === 0 || shake) titleDraw(t);
      ctx.drawImage(titleCv, 0, 0);
      loading(elapsed, t);
      if (k >= P_WASH - 0.04) wash(Math.min(1, (k - P_WASH + 0.04) / 0.12), t);
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      t++;
    };
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, 320, 240);
    (document.fonts ? document.fonts.load("8px 'Press Start 2P'") : Promise.resolve()).finally(() => {
      raf = requestAnimationFrame(frame);
    });
    return () => cancelAnimationFrame(raf);
  }, [title, tagline, duration, loop, finale]);
  return React.createElement('canvas', {
    ref,
    width: 320,
    height: 240,
    style: {
      width: 320 * scale,
      height: 240 * scale,
      imageRendering: 'pixelated',
      display: 'block',
      background: '#000',
      ...style
    }
  });
}
Object.assign(__ds_scope, { BootScene });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/device/BootScene.jsx", error: String((e && e.message) || e) }); }

// components/icons/UnicornSprite.jsx
try { (() => {
function UnicornSprite({
  cell = 3,
  animate = true,
  frame,
  style
}) {
  const ref = React.useRef(null);
  const t = __ds_scope.useTick(animate);
  const W = 40,
    H = 29;
  React.useEffect(() => {
    const ctx = ref.current && ref.current.getContext('2d');
    if (!ctx) return;
    ctx.clearRect(0, 0, W * cell, H * cell);
    const f = frame ?? Math.floor(t / 4) % 2;
    __ds_scope.UNICORN_FRAMES[f].forEach((row, y) => {
      const mane = __ds_scope.cyberHue(y * 18 + t * 12);
      [...row].forEach((k, x) => {
        if (k === '.') return;
        ctx.fillStyle = __ds_scope.css(__ds_scope.to565(k === 'R' ? mane : __ds_scope.UNICORN_PALETTE[k]));
        ctx.fillRect(x * cell, y * cell, cell, cell);
      });
    });
  }, [t, cell, frame]);
  return React.createElement('canvas', {
    ref,
    width: W * cell,
    height: H * cell,
    role: 'img',
    'aria-label': 'CyberUnicorn',
    style: {
      imageRendering: 'pixelated',
      display: 'block',
      ...style
    }
  });
}
Object.assign(__ds_scope, { UnicornSprite });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/icons/UnicornSprite.jsx", error: String((e && e.message) || e) }); }

// components/rainbow/RainbowBar.jsx
try { (() => {
// cyberRainbowFill() progress bar with the boot screen's cyan outline.
function RainbowBar({
  value = 0.5,
  width = 288,
  height = 10,
  phase = 0,
  animate = false,
  label,
  showPercent = true,
  style
}) {
  const [p, setP] = React.useState(phase);
  React.useEffect(() => {
    if (!animate) return;
    let raf;
    const loop = () => {
      setP(Math.floor(performance.now() / 40) * 10 % 360);
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [animate]);
  const ph = animate ? p : phase;
  const pct = Math.round(value * 100);
  const inner = width - 2,
    done = Math.round(Math.min(1, value) * inner);
  const stops = [];
  for (let x = 0; x <= done; x += 6) stops.push(`${__ds_scope.css(__ds_scope.to565(__ds_scope.cyberHue(x * 2 - ph)))} ${x}px`);
  const over = pct >= 100;
  return React.createElement('div', {
    style: {
      width,
      fontFamily: 'var(--font-display)',
      fontSize: 6,
      ...style
    }
  }, React.createElement('div', {
    style: {
      width,
      height,
      boxSizing: 'border-box',
      border: '1px solid var(--cu-cyan)'
    }
  }, React.createElement('div', {
    style: {
      width: done,
      height: height - 2,
      background: `linear-gradient(90deg, ${stops.join(',') || 'transparent'})`
    }
  })), (label || showPercent) && React.createElement('div', {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      marginTop: 5,
      color: 'var(--cu-white)',
      lineHeight: '8px'
    }
  }, React.createElement('span', null, label || ''), showPercent && React.createElement('span', {
    style: {
      color: over ? __ds_scope.css(__ds_scope.cyberHue(ph * 2)) : 'var(--cu-magenta)'
    }
  }, over ? `${pct}% (oops)` : `${pct}%`)));
}
Object.assign(__ds_scope, { RainbowBar });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/rainbow/RainbowBar.jsx", error: String((e && e.message) || e) }); }

// components/rainbow/RainbowFrame.jsx
try { (() => {
// cyberRainbowFrame(): an outline whose colour walks the hue wheel around the perimeter.
function RainbowFrame({
  children,
  thickness = 1,
  phase = 0,
  animate = false,
  radius = 0,
  padding = 8,
  style
}) {
  const [p, setP] = React.useState(phase);
  React.useEffect(() => {
    if (!animate) return;
    let raf;
    const loop = () => {
      setP(Math.floor(performance.now() / 25) % 360);
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [animate]);
  const ph = animate ? p : phase;
  const stops = Array.from({
    length: 13
  }, (_, i) => __ds_scope.css(__ds_scope.to565(__ds_scope.cyberHue(ph + i * 30))) + ` ${i * 30}deg`).join(',');
  return React.createElement('div', {
    style: {
      position: 'relative',
      padding: thickness,
      borderRadius: radius,
      background: `conic-gradient(from 90deg, ${stops})`,
      ...style
    }
  }, React.createElement('div', {
    style: {
      background: 'var(--surface-screen)',
      borderRadius: Math.max(0, radius - thickness),
      padding,
      height: '100%',
      boxSizing: 'border-box'
    }
  }, children));
}
Object.assign(__ds_scope, { RainbowFrame });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/rainbow/RainbowFrame.jsx", error: String((e && e.message) || e) }); }

// components/rainbow/RainbowText.jsx
try { (() => {
const hex = c => __ds_scope.css(__ds_scope.to565(c));
// cyberRainbowText(): each letter a different hue (phase + i*25).
function RainbowText({
  children = '',
  phase = 0,
  size = 2,
  animate = false,
  as = 'span',
  style
}) {
  const [p, setP] = React.useState(phase);
  React.useEffect(() => {
    if (!animate) return;
    let raf;
    const loop = () => {
      setP(Math.floor(performance.now() / 25) % 360);
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [animate]);
  const textOf = c => c == null || c === false ? '' : typeof c === 'string' || typeof c === 'number' ? String(c) : Array.isArray(c) ? c.map(textOf).join('') : c.props ? textOf(c.props.children) : '';
  const text = textOf(children);
  const fs = 6 * size;
  return React.createElement(as, {
    style: {
      fontFamily: 'var(--font-display)',
      fontSize: fs,
      lineHeight: `${8 * size}px`,
      letterSpacing: 0,
      whiteSpace: 'pre',
      ...style
    }
  }, [...text].map((ch, i) => React.createElement('span', {
    key: i,
    style: {
      color: hex(__ds_scope.cyberHue((animate ? p : phase) + i * 25))
    }
  }, ch)));
}
Object.assign(__ds_scope, { RainbowText });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/rainbow/RainbowText.jsx", error: String((e && e.message) || e) }); }

// components/device/AppFrame.jsx
try { (() => {
const GREY = '#7b7b7b'; // TFT_DARKGREY

function StatusBar({
  phase = 0,
  label = 'CU5K 0.1.0',
  clock = '16:35',
  link = 'USB'
}) {
  const line = Array.from({
    length: 7
  }, (_, i) => __ds_scope.hueCss(phase + i * 60)).join(',');
  return React.createElement('div', {
    style: {
      position: 'absolute',
      left: 3,
      right: 3,
      top: 3,
      height: 20,
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      padding: '0 6px',
      fontFamily: "'Press Start 2P', monospace",
      fontSize: 6
    }
  }, React.createElement(__ds_scope.RainbowText, {
    size: 1,
    phase
  }, label), React.createElement('span', {
    style: {
      display: 'flex',
      gap: 8
    }
  }, React.createElement('span', {
    style: {
      color: 'var(--cu-cyan)'
    }
  }, link), React.createElement('span', {
    style: {
      color: '#fff'
    }
  }, clock)), React.createElement('div', {
    style: {
      position: 'absolute',
      left: 0,
      right: 0,
      bottom: -1,
      height: 1,
      background: `linear-gradient(90deg, ${line})`
    }
  }));
}

// drawMainBorderWithTitle() with the rainbow skin: border, status bar, rainbow
// title at y 28, content from y 46 (TOP), dark-grey footer at y 206.
function AppFrame({
  title = '',
  footer,
  children,
  animate = true,
  phase: fixed,
  statusLabel,
  clock,
  link,
  background = '#000',
  backdrop,
  footerColor
}) {
  const live = __ds_scope.usePhase(animate);
  const phase = fixed ?? live;
  const stops = Array.from({
    length: 13
  }, (_, i) => `${__ds_scope.hueCss(phase + i * 30)} ${i * 30}deg`).join(',');
  return React.createElement('div', {
    style: {
      position: 'absolute',
      inset: 0,
      background,
      overflow: 'hidden'
    }
  }, backdrop, React.createElement('div', {
    style: {
      position: 'absolute',
      inset: 1,
      borderRadius: 5,
      padding: 1,
      background: `conic-gradient(from 90deg, ${stops})`,
      WebkitMask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)',
      mask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)'
    }
  }), React.createElement(StatusBar, {
    phase,
    label: statusLabel,
    clock,
    link
  }), title && React.createElement('div', {
    style: {
      position: 'absolute',
      top: 28,
      left: 0,
      right: 0,
      display: 'flex',
      justifyContent: 'center'
    }
  }, React.createElement(__ds_scope.RainbowText, {
    size: 2,
    phase
  }, title)), children, footer && React.createElement('div', {
    style: {
      position: 'absolute',
      top: 206,
      left: 0,
      right: 0,
      textAlign: 'center',
      fontFamily: "'Press Start 2P', monospace",
      fontSize: 6,
      lineHeight: '8px',
      color: footerColor || GREY,
      whiteSpace: 'pre'
    }
  }, footer));
}
Object.assign(__ds_scope, { StatusBar, AppFrame });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/device/AppFrame.jsx", error: String((e && e.message) || e) }); }

// components/device/MenuScreen.jsx
try { (() => {
// Main menu with the rainbow skin: one big icon, label, prev/next arrows.
// Layout is an approximation of Bruce 1.16.1's main menu, not a pixel copy.
function MenuScreen({
  app = 'frequency',
  index,
  count,
  onPrev,
  onNext,
  onSelect,
  status = 'CU5K 0.1.0',
  clock = '16:35',
  animate = true
}) {
  const icon = __ds_scope.ICONS[app];
  const [ph, setPh] = React.useState(0);
  React.useEffect(() => {
    if (!animate) return;
    let raf;
    const loop = () => {
      setPh(Math.floor(performance.now() / 25) % 360);
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [animate]);
  const stops = Array.from({
    length: 13
  }, (_, i) => __ds_scope.css(__ds_scope.to565(__ds_scope.cyberHue(ph + i * 30))) + ` ${i * 30}deg`).join(',');
  const arrow = (dir, fn) => React.createElement('button', {
    onClick: fn,
    'aria-label': dir < 0 ? 'Previous' : 'Next',
    style: {
      all: 'unset',
      cursor: 'pointer',
      width: 28,
      height: 72,
      display: 'grid',
      placeItems: 'center'
    }
  }, React.createElement('div', {
    style: {
      width: 0,
      height: 0,
      borderTop: '9px solid transparent',
      borderBottom: '9px solid transparent',
      [dir < 0 ? 'borderRight' : 'borderLeft']: '9px solid var(--cu-magenta)'
    }
  }));
  return React.createElement('div', {
    style: {
      position: 'absolute',
      inset: 0,
      background: '#000',
      fontFamily: 'var(--font-display)',
      color: '#fff'
    }
  }, React.createElement(__ds_scope.StatusBar, {
    phase: ph,
    label: status,
    clock
  }), React.createElement('div', {
    style: {
      position: 'absolute',
      left: 4,
      right: 4,
      top: 24,
      bottom: 4,
      padding: 1,
      background: `conic-gradient(from 90deg, ${stops})`,
      borderRadius: 5
    }
  }, React.createElement('div', {
    style: {
      width: '100%',
      height: '100%',
      background: '#000',
      borderRadius: 4,
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      justifyContent: 'center',
      gap: 14
    }
  }, React.createElement('div', {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 18
    }
  }, arrow(-1, onPrev), React.createElement('button', {
    onClick: onSelect,
    style: {
      all: 'unset',
      cursor: 'pointer'
    }
  }, React.createElement(__ds_scope.PixelIcon, {
    name: app,
    cell: 3,
    animate
  })), arrow(1, onNext)), React.createElement(__ds_scope.RainbowText, {
    size: 2,
    phase: ph
  }, icon ? icon.name : app), index !== undefined && count ? React.createElement('div', {
    style: {
      display: 'flex',
      gap: 3
    }
  }, Array.from({
    length: count
  }, (_, i) => React.createElement('div', {
    key: i,
    style: {
      width: 3,
      height: 3,
      background: i === index ? __ds_scope.css(__ds_scope.to565(__ds_scope.cyberHue(300 + i * 30))) : '#301040'
    }
  }))) : null)));
}
Object.assign(__ds_scope, { MenuScreen });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/device/MenuScreen.jsx", error: String((e && e.message) || e) }); }

// components/device/OptionList.jsx
try { (() => {
// cyberItemColor(), lifted toward white in the deep-blue band (200-290 deg) so
// every row stays readable on black (pure 0,0,255 is ~2:1).
function itemColor(h) {
  const d = (h % 360 + 360) % 360,
    [r, g, b] = __ds_scope.cyberHue(d);
  const k = d >= 200 && d <= 290 ? 0.45 : 0;
  return __ds_scope.css(__ds_scope.to565([r + (255 - r) * k | 0, g + (255 - g) * k | 0, b + (255 - b) * k | 0]));
}
// loopOptions(MENU_TYPE_SUBMENU) with the rainbow skin: each option its own neon
// colour (cyberItemColor), the selected one inverted in a rainbow-framed box.
function OptionList({
  title,
  options = [],
  selected = 0,
  onSelect,
  onHover,
  animate = true,
  top = 46,
  maxRows = 7,
  background = '#000'
}) {
  const phase = __ds_scope.usePhase(animate);
  const step = Math.max(30, Math.floor(360 / Math.max(1, options.length)));
  const first = Math.max(0, Math.min(selected - Math.floor(maxRows / 2), options.length - maxRows));
  const shown = options.slice(first, first + maxRows);
  const stops = Array.from({
    length: 13
  }, (_, i) => `${__ds_scope.hueCss(phase + i * 30)} ${i * 30}deg`).join(',');
  return React.createElement('div', {
    style: {
      position: 'absolute',
      left: 20,
      right: 20,
      top: title ? top - 18 : top,
      fontFamily: "'Press Start 2P', monospace"
    }
  }, title && React.createElement('div', {
    style: {
      display: 'flex',
      justifyContent: 'center',
      marginBottom: 6
    }
  }, React.createElement(__ds_scope.RainbowText, {
    size: 1,
    phase
  }, title)), React.createElement('div', {
    style: {
      position: 'relative',
      padding: 1,
      borderRadius: 4
    }
  }, React.createElement('div', {
    style: {
      position: 'absolute',
      inset: 0,
      borderRadius: 4,
      padding: 1,
      background: `conic-gradient(from 90deg, ${stops})`,
      WebkitMask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)',
      mask: 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)'
    }
  }), React.createElement('div', {
    style: {
      position: 'relative',
      background,
      borderRadius: 3,
      padding: '4px 0'
    }
  }, shown.map((o, j) => {
    const i = first + j,
      on = i === selected,
      label = typeof o === 'string' ? o : o.label,
      c = itemColor(300 + i * step);
    return React.createElement('button', {
      key: i,
      onClick: () => onSelect && onSelect(i),
      onMouseEnter: () => onHover && onHover(i),
      style: {
        all: 'unset',
        cursor: 'pointer',
        display: 'flex',
        justifyContent: 'space-between',
        width: '100%',
        boxSizing: 'border-box',
        height: 16,
        alignItems: 'center',
        padding: '0 8px',
        fontSize: 6,
        color: on ? '#000' : c,
        background: on ? c : 'transparent'
      }
    }, React.createElement('span', null, (on ? '> ' : '  ') + label), typeof o === 'object' && o.value != null && React.createElement('span', null, o.value));
  }))), options.length > maxRows && React.createElement('div', {
    style: {
      textAlign: 'center',
      fontSize: 6,
      color: '#7b7b7b',
      marginTop: 4
    }
  }, `${selected + 1}/${options.length}`));
}
Object.assign(__ds_scope, { OptionList });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/device/OptionList.jsx", error: String((e && e.message) || e) }); }

// icons/build_icons.js
try { (() => {
// CyberUnicorn 5000 app icons — source of truth.
// Builds every icon as a 32x24 character grid on the boot unicorn's palette
// (make_unicorn.py), plus four animated indices. Run by the project's build
// script to regenerate components/icons/iconData.js and icons/cyber_icons.h.
// Hand-tune by editing the shape calls below, never the generated files.
function buildIcons() {
  const W = 32,
    H = 24;
  // key: [index, rgb | special]. 0-9 are identical to unicorn_sprite.h.
  const PALETTE = [['.', null, 'transparent'], ['R', 'rainbow', 'animated rainbow (hue by icon mode + time)'], ['K', [24, 6, 40], 'outline, deep purple'], ['W', [255, 255, 255], 'body white'], ['w', [200, 180, 255], 'body shade, lavender'], ['H', [255, 220, 60], 'horn / sun yellow'], ['h', [255, 140, 0], 'horn shade, orange'], ['E', [0, 255, 255], 'cyber eye, neon cyan'], ['P', [255, 90, 200], 'hot pink'], ['G', [0, 255, 160], 'neon green (hooves)'], ['M', [255, 0, 200], 'magenta (grid rays, title shadow)'], ['B', [0, 160, 255], 'grid blue'], ['D', [30, 0, 70], 'sky top / panel'], ['V', [96, 0, 48], 'sky low / dim LED'], ['N', [12, 0, 28], 'grid floor / screen'], ['o', [255, 150, 120], 'sun mid'], ['S', 'sparkle', 'twinkle: white, cyan, pink, off'], ['A', 'frameA', 'rainbow, frame A only'], ['a', 'frameB', 'rainbow, frame B only'], ['Q', 'gridA', 'grid blue, frame A only'], ['q', 'gridB', 'grid blue, frame B only']];
  const icons = {};
  function make(id, name, mode, draw, note) {
    const g = Array.from({
      length: H
    }, () => Array(W).fill('.'));
    const T = {
      p(x, y, c) {
        x = Math.round(x);
        y = Math.round(y);
        if (x >= 0 && y >= 0 && x < W && y < H) g[y][x] = c;
      },
      get(x, y) {
        return x >= 0 && y >= 0 && x < W && y < H ? g[y][x] : '.';
      },
      rect(x0, y0, x1, y1, c) {
        for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) T.p(x, y, c);
      },
      box(x0, y0, x1, y1, c) {
        for (let x = x0; x <= x1; x++) {
          T.p(x, y0, c);
          T.p(x, y1, c);
        }
        for (let y = y0; y <= y1; y++) {
          T.p(x0, y, c);
          T.p(x1, y, c);
        }
      },
      rrect(x0, y0, x1, y1, c, r = 1) {
        for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) {
          const dx = x < x0 + r ? x0 + r - x : x > x1 - r ? x - (x1 - r) : 0;
          const dy = y < y0 + r ? y0 + r - y : y > y1 - r ? y - (y1 - r) : 0;
          if (dx * dx + dy * dy <= r * r) T.p(x, y, c);
        }
      },
      hl(x0, y, x1, c) {
        for (let x = x0; x <= x1; x++) T.p(x, y, c);
      },
      vl(x, y0, y1, c) {
        for (let y = y0; y <= y1; y++) T.p(x, y, c);
      },
      disc(cx, cy, r, c, fn) {
        for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
          const d = (x - cx) ** 2 + (y - cy) ** 2;
          if (d <= r * r + r * 0.6 && (!fn || fn(x, y))) T.p(x, y, c);
        }
      },
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
        add.forEach(([x, y]) => g[y][x] = c);
      },
      glyph(rows, x0, y0, c) {
        rows.forEach((r, y) => [...r].forEach((ch, x) => ch === 'X' && T.p(x0 + x, y0 + y, c)));
      }
    };
    draw(T);
    icons[id] = {
      id,
      name,
      mode,
      note,
      grid: g.map(r => r.join(''))
    };
  }

  // ---- CyberUnicorn apps (apps/apps.h order) ----
  // The Frequency logo, traced from uploads/Frequency-Logo-Round-Icon.png at 24 px:
  // ring + FR ligature (F stem and top bar running into the R bowl, short F
  // crossbar, R stem and diagonal leg). Synthwave version: rainbow ring, night-sky
  // disc, white FR, striped sun rising behind the legs.
  make('frequency', 'Frequency Check-in', 'angle', T => {
    const cx = 15.5,
      cy = 11.5,
      d = (x, y) => Math.hypot(x - cx, y - cy);
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) if (d(x, y) < 9.7) T.p(x, y, y < 8 ? 'D' : y < 14 ? 'V' : 'N');
    // striped sun rising from the bottom of the disc
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) {
      if (d(x, y) >= 9.7 || Math.hypot(x - 15.5, y - 21) > 7.2 || y > 20) continue;
      if (y === 17 || y === 19) continue;
      T.p(x, y, y <= 15 ? 'H' : y <= 16 ? 'h' : y <= 18 ? 'o' : 'P');
    }
    [[10, 4], [21, 3], [24, 9], [7, 10]].forEach(([x, y]) => T.p(x, y, 'S'));
    const FR = ['.XXXXXXXXXXX...', 'XXXXXXXXXXXXX..', 'XX.........XXX.', 'XX..........XX.', 'XXXXX.XXXXXXXX.', 'XXXXX.XXXXXXX..', 'XX....XX..XX...', 'XX....XX...XX..', 'XX....XX...XX..', 'XX....XX....XX.', 'XX....XX.....XX'];
    const ox = 8,
      oy = 6;
    FR.forEach((r, j) => [...r].forEach((c, i) => c === 'X' && T.p(ox + i, oy + j, j >= 9 ? 'w' : 'W')));
    T.ring(cx, cy, 10, 10, 'K');
    T.ring(cx, cy, 11, 11.6, 'R');
    T.p(2, 2, 'S');
    T.p(29, 3, 'S');
    T.p(29, 21, 'S');
    T.p(2, 21, 'S');
    [[3, 6], [28, 16]].forEach(([x, y], i) => {
      const c = i ? 'a' : 'A';
      T.p(x, y, c);
      T.p(x - 1, y, c);
      T.p(x + 1, y, c);
      T.p(x, y - 1, c);
      T.p(x, y + 1, c);
    });
  }, 'the Frequency FR logo: rainbow ring, white FR on a night-sky disc, striped sun rising');
  make('lightshow', 'LED Light Show', 'diag', T => {
    const heart = ['.XX..XX.', 'XXXXXXXX', 'XXXXXXXX', '.XXXXXX.', '..XXXX..', '...XX...', '........'];
    T.rrect(3, 0, 28, 21, 'D', 1);
    const on = (c, r) => heart[r] && heart[r][c] === 'X';
    for (let r = 0; r < 7; r++) for (let c = 0; c < 8; c++) {
      const x = 5 + c * 3,
        y = 1 + r * 3;
      let ch = 'N';
      if (on(c, r)) ch = [[1, 0], [-1, 0], [0, 1], [0, -1]].every(([dx, dy]) => on(c + dx, r + dy)) ? 'R' : 'A';
      T.rect(x, y, x + 1, y + 1, ch);
    }
    T.p(26, 19, 'S');
    T.p(5, 19, 'S');
    T.outline(['D']);
    T.rect(14, 23, 17, 23, 'w');
  }, '8x8 LED panel; rainbow heart beats');
  make('sensors', 'Sensor Hub', 'col', T => {
    T.rrect(1, 1, 30, 20, 'w', 2);
    T.rect(3, 3, 28, 18, 'N');
    [7, 11, 15].forEach(y => T.hl(3, y, 28, 'D'));
    [8, 13, 18, 23].forEach(x => T.vl(x, 3, 18, 'D'));
    const ys = [15, 11, 13, 7, 10, 5, 8, 6];
    for (let i = 1; i < ys.length; i++) {
      const x0 = 4 + (i - 1) * 3,
        x1 = 4 + i * 3;
      T.line(x0, ys[i - 1], x1, ys[i], 'R');
      T.line(x0, ys[i - 1] + 1, x1, ys[i] + 1, 'R');
    }
    T.rect(25, 5, 26, 7, 'A');
    T.p(25, 15, 'G');
    T.p(27, 15, 'G');
    T.p(25, 17, 'a');
    T.p(27, 17, 'a');
    T.rect(12, 21, 19, 22, 'w');
    T.outline(['w']);
  }, 'little screen with a rainbow sparkline and node dots');
  make('led', 'RGB LED', 'row', T => {
    T.disc(16, 9, 6, 'R', (x, y) => y <= 9);
    T.rect(10, 9, 22, 14, 'R');
    T.vl(12, 6, 11, 'W');
    T.p(13, 4, 'W');
    T.p(13, 5, 'W');
    T.rect(9, 15, 23, 16, 'w');
    T.vl(13, 17, 23, 'w');
    T.vl(19, 17, 21, 'w');
    T.p(20, 21, 'w');
    T.outline(['R', 'W', 'w']);
    for (let k = 0; k < 8; k++) {
      const a = -180 + k * 45;
      if (a > 10 && a < 170) continue;
      const c = k % 2 ? 'a' : 'A';
      const r = Math.PI / 180;
      for (let d = 10; d <= 12; d++) T.p(16 + Math.cos(a * r) * d, 9 + Math.sin(a * r) * d, c);
    }
    T.p(3, 20, 'S');
    T.p(28, 18, 'S');
  }, '5 mm LED: rainbow dome, glow rays');
  make('light', 'Light Meter', 'angle', T => {
    T.disc(16, 12, 8, 'H');
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) if (T.get(x, y) === 'H') {
      if (y >= 7) T.p(x, y, 'h');
      if (y >= 11) T.p(x, y, 'o');
      if (y >= 15) T.p(x, y, 'P');
    }
    [13, 16, 19].forEach(y => T.hl(0, y, 31, '.'));
    T.outline(['H', 'h', 'o', 'P']);
    for (let k = 0; k < 12; k++) {
      const a = k * 30 * Math.PI / 180,
        c = k % 2 ? 'a' : 'A';
      for (let d = 11; d <= 13; d++) T.p(16 + Math.cos(a) * d, 12 + Math.sin(a) * d, c);
    }
  }, 'the boot screen\'s striped synthwave sun; rays spin');
  make('sound', 'Sound & Tones', 'row', T => {
    T.rect(3, 9, 7, 14, 'w');
    for (let i = 0; i < 6; i++) T.vl(8 + i, 9 - i, 14 + i, 'W');
    T.vl(14, 3, 20, 'w');
    T.vl(9, 9, 14, 'w');
    T.outline(['W', 'w']);
    T.ring(13, 11.5, 5, 6, 'R', -40, 40);
    T.ring(13, 11.5, 9, 10, 'A', -45, 45);
    T.ring(13, 11.5, 13, 14, 'a', -48, 48);
    T.p(29, 2, 'S');
    T.p(30, 21, 'S');
  }, 'speaker cone, rainbow waves pulse out');
  make('slideshow', 'Photo Slideshow', 'angle', T => {
    T.box(0, 0, 31, 23, 'R');
    T.box(1, 1, 30, 22, 'R');
    T.rect(2, 2, 29, 7, 'D');
    T.rect(2, 8, 29, 12, 'V');
    T.disc(21, 13, 6, 'H', (x, y) => y <= 12 && y >= 2);
    for (let y = 2; y <= 12; y++) for (let x = 2; x < 30; x++) if (T.get(x, y) === 'H') {
      if (y >= 9) T.p(x, y, 'h');
      if (y >= 11) T.p(x, y, 'P');
    }
    [10, 12].forEach(y => {
      for (let x = 14; x < 29; x++) if ('Hh P'.includes(T.get(x, y)) && T.get(x, y) !== ' ') T.p(x, y, 'V');
    });
    T.p(5, 4, 'S');
    T.p(10, 3, 'S');
    T.p(14, 6, 'S');
    T.p(27, 4, 'S');
    T.rect(2, 14, 29, 21, 'N');
    for (let k = -4; k <= 4; k++) T.line(16 + k, 13, 16 + k * 5, 21, 'M');
    [15, 17, 20].forEach(y => T.hl(2, y, 29, 'B'));
    T.hl(2, 13, 29, 'M');
  }, 'rainbow picture frame around a tiny boot scene');
  make('weather', 'Weather Forecast', 'col', T => {
    T.disc(21, 7, 5, 'H');
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) if (T.get(x, y) === 'H' && y >= 8) T.p(x, y, 'h');
    T.outline(['H', 'h']);
    T.disc(11, 13, 4, 'W');
    T.disc(17, 11, 5, 'W');
    T.disc(23, 14, 3, 'W');
    T.rect(8, 13, 25, 17, 'W');
    T.hl(8, 17, 25, 'w');
    T.hl(10, 16, 13, 'w');
    T.p(26, 15, 'w');
    T.outline(['W', 'w']);
    [[29, 3], [27, 0], [30, 8]].forEach(([x, y], i) => T.p(x, y, i % 2 ? 'a' : 'A'));
    [9, 12, 15, 18, 21, 24].forEach((x, i) => {
      const o = i % 2;
      T.vl(x, 19 + o, 20 + o, o ? 'a' : 'A');
      T.vl(x - 1, 21 + o, 22 + o, o ? 'A' : 'a');
    });
  }, 'sun behind a cloud, rainbow rain falls');
  make('hat', 'Hat Module Probe', 'row', T => {
    T.rrect(1, 5, 30, 19, 'D', 1);
    for (let x = 3; x <= 28; x += 2) {
      T.p(x, 4, 'H');
      T.p(x, 3, 'h');
    }
    T.hl(3, 18, 28, 'V');
    T.vl(3, 7, 18, 'V');
    T.vl(28, 7, 18, 'V');
    T.rect(6, 8, 25, 16, 'M');
    for (let i = 0; i < 6; i++) {
      const sx = 8 + i * 3;
      T.rect(sx, 9, sx + 1, 14, 'N');
      if (i === 2) {
        T.rect(sx, 9, sx + 1, 11, 'A');
        T.rect(sx, 12, sx + 1, 14, 'a');
      } else T.rect(sx, 12, sx + 1, 14, 'W');
      T.p(sx, 16, 'W');
    }
    T.outline(['D', 'H', 'h']);
  }, 'the NM-RF-HAT with its DIP switch; DIP 3 (PN532) flips');
  make('gpio', 'GPIO Pins', 'row', T => {
    T.rect(9, 2, 22, 21, 'D');
    T.box(9, 2, 22, 21, 'w');
    T.hl(10, 3, 21, 'V');
    T.rect(11, 4, 12, 5, 'P');
    const three = ['XXX', '..X', 'XXX', '..X', 'XXX'],
      two = ['XXX', '..X', 'XXX', 'X..', 'XXX'];
    T.glyph(three, 12, 10, 'E');
    T.glyph(two, 16, 10, 'E');
    [3, 7, 11, 15, 19].forEach((y, i) => {
      T.rect(3, y, 8, y + 1, 'R');
      T.rect(23, y, 28, y + 1, 'R');
      T.rect(1, y, 2, y + 1, i % 2 ? 'a' : 'A');
      T.rect(29, y, 30, y + 1, i % 2 ? 'A' : 'a');
    });
    T.outline(['w', 'R']);
  }, 'ESP32 chip with rainbow legs; signals hop pin to pin');
  make('remote', 'Phone Remote', 'row', T => {
    T.rrect(2, 1, 13, 22, 'w', 2);
    T.rect(4, 4, 11, 17, 'N');
    T.rect(4, 6, 11, 15, 'R');
    T.hl(6, 2, 9, 'D');
    T.rect(7, 19, 8, 20, 'D');
    T.outline(['w']);
    [[17, 3], [20, 6], [23, 9], [26, 12]].forEach(([x, h], i) => {
      T.rect(x, 18 - h, x + 1, 18, i < 3 ? 'R' : 'a');
    });
    T.outline(['R'].concat([]));
    T.p(29, 2, 'S');
    T.p(22, 3, 'S');
  }, 'phone with a rainbow screen; signal bars fill');

  // ---- Bruce's built-in menus (re-skinned) ----
  make('wifi', 'WiFi Tools', 'row', T => {
    T.rect(15, 19, 17, 21, 'W');
    T.outline(['W']);
    T.ring(16, 20, 4, 5, 'R', -140, -40);
    T.ring(16, 20, 8, 9, 'R', -140, -40);
    T.ring(16, 20, 12, 13, 'A', -140, -40);
    T.ring(16, 20, 16, 17, 'a', -140, -40);
    T.p(3, 3, 'S');
    T.p(29, 20, 'S');
  }, 'fan of arcs, outer two pulse');
  make('ble', 'Bluetooth LE', 'angle', T => {
    T.rrect(5, 4, 26, 19, 'B', 4);
    T.hl(7, 18, 24, 'D');
    T.glyph(['XXX.', 'X..X', 'XXX.', 'X..X', 'X..X', 'XXX.'], 8, 8, 'W');
    T.glyph(['X...', 'X...', 'X...', 'X...', 'X...', 'XXX.'], 13, 8, 'W');
    T.glyph(['XXX', 'X..', 'XX.', 'X..', 'X..', 'XXX'], 18, 8, 'W');
    T.rect(22, 7, 23, 8, 'A');
    T.outline(['B', 'D']);
    T.ring(16, 12, 11, 11, 'A', 150, -150);
    T.ring(16, 12, 11, 11, 'A', -30, 30);
    T.ring(16, 12, 14, 14, 'a', 150, -150);
    T.ring(16, 12, 14, 14, 'a', -30, 30);
  }, 'blue "BLE" badge with broadcast arcs (no Bluetooth rune: trademark)');
  make('rf', 'Sub-GHz Radio', 'row', T => {
    T.rect(15, 9, 16, 19, 'w');
    T.hl(13, 20, 18, 'w');
    T.hl(12, 21, 19, 'w');
    T.hl(11, 22, 20, 'w');
    T.rrect(13, 4, 18, 8, 'P', 1);
    T.outline(['w', 'P']);
    [[5, 6, 'R'], [9, 10, 'A'], [13, 14, 'a']].forEach(([r0, r1, c]) => {
      T.ring(15.5, 6, r0, r1, c, -45, 45);
      T.ring(15.5, 6, r0, r1, c, 135, -135);
    });
  }, 'Sub-GHz antenna mast radiating both ways');
  make('rfid', 'RFID Cards', 'col', T => {
    T.rrect(1, 4, 25, 20, 'W', 2);
    T.hl(3, 20, 23, 'w');
    T.vl(25, 6, 18, 'w');
    T.rect(4, 8, 10, 13, 'H');
    T.hl(4, 10, 10, 'h');
    T.vl(7, 8, 13, 'h');
    T.hl(13, 9, 22, 'w');
    T.hl(13, 12, 19, 'w');
    T.rect(2, 16, 24, 17, 'R');
    T.outline(['W', 'w']);
    T.ring(22, 12, 6, 6, 'A', -35, 35);
    T.ring(22, 12, 8, 8, 'a', -40, 40);
  }, 'card with a gold chip and holo stripe');
  make('ir', 'Infrared Remote', 'col', T => {
    T.rrect(1, 8, 15, 16, 'w', 2);
    T.rect(4, 10, 5, 11, 'P');
    T.rect(8, 10, 9, 11, 'E');
    T.rect(4, 13, 5, 14, 'G');
    T.rect(8, 13, 9, 14, 'H');
    T.rect(12, 11, 13, 13, 'D');
    T.rect(16, 11, 17, 13, 'M');
    T.outline(['w', 'M']);
    for (let k = 0; k < 4; k++) {
      const x = 20 + k * 3,
        c = k % 2 ? 'a' : 'A';
      for (let y = 12 - k; y <= 12 + k; y++) if (y === 12 - k || y === 12 + k || y === 12) {
        T.p(x, y, c);
        T.p(x + 1, y, c);
      }
    }
  }, 'remote firing a rainbow beam');
  make('nrf24', '2.4 GHz Radio', 'row', T => {
    T.rrect(2, 8, 29, 21, 'D', 1);
    T.hl(4, 10, 10, 'H');
    T.vl(10, 10, 13, 'H');
    T.hl(4, 13, 10, 'H');
    T.vl(4, 13, 16, 'H');
    T.hl(4, 16, 10, 'H');
    T.vl(10, 16, 19, 'H');
    T.hl(4, 19, 10, 'H');
    T.rect(14, 11, 20, 17, 'N');
    T.p(15, 12, 'E');
    T.rect(14, 19, 17, 19, 'w');
    for (let y = 10; y <= 18; y += 2) {
      T.p(24, y, 'H');
      T.p(26, y, 'H');
    }
    T.outline(['D']);
    T.ring(7, 8, 3, 3, 'R', -150, -30);
    T.ring(7, 8, 5, 5, 'A', -150, -30);
    T.ring(7, 8, 7, 7, 'a', -150, -30);
  }, '2.4 GHz module with meander antenna');
  make('gps', 'GPS & Wardriving', 'col', T => {
    for (let k = -5; k <= 5; k++) T.line(16 + k * 2, 14, 16 + k * 6, 23, 'M');
    [15, 17, 20, 23].forEach(y => T.hl(0, y, 31, 'B'));
    for (let a = 0; a < 360; a += 6) {
      const r = a * Math.PI / 180;
      T.p(16 + Math.cos(r) * 6, 20 + Math.sin(r) * 2, 'A');
      T.p(16 + Math.cos(r) * 10, 20 + Math.sin(r) * 3, 'a');
    }
    T.disc(16, 7, 6, 'P');
    for (let y = 8; y <= 19; y++) {
      const hw = Math.round(6 * (19 - y) / 11);
      T.hl(16 - hw, y, 16 + hw, 'P');
    }
    T.disc(16, 7, 2, 'W');
    T.p(12, 4, 'w');
    T.p(11, 5, 'w');
    T.p(11, 6, 'w');
    T.outline(['P']);
  }, 'map pin landing on the synthwave grid');
  make('scripts', 'JS Scripts', 'col', T => {
    T.rect(1, 2, 30, 21, 'N');
    T.box(1, 2, 30, 21, 'w');
    T.rect(2, 3, 29, 4, 'R');
    T.glyph(['X..', '.X.', '..X', '.X.', 'X..'], 3, 7, 'G');
    T.hl(8, 9, 14, 'E');
    T.hl(16, 9, 22, 'w');
    T.hl(4, 13, 9, 'P');
    T.hl(11, 13, 20, 'w');
    T.hl(4, 16, 10, 'H');
    T.rect(12, 15, 13, 17, 'A');
    T.outline(['w']);
  }, 'terminal with rainbow title bar; cursor blinks');
  make('others', 'More Tools', 'angle', T => {
    for (let y = 0; y < 24; y++) for (let x = 0; x < 32; x++) {
      const dx = Math.abs(x - 16),
        dy = Math.abs(y - 12);
      if (Math.sqrt(dx) + Math.sqrt(dy) <= Math.sqrt(11)) T.p(x, y, 'R');
    }
    T.p(16, 12, 'W');
    T.p(15, 12, 'W');
    T.p(16, 11, 'W');
    T.outline(['R', 'W']);
    [[5, 4], [27, 19], [26, 4], [5, 19]].forEach(([x, y], i) => {
      const c = i % 2 ? 'a' : 'A';
      T.p(x, y, c);
      T.p(x - 1, y, c);
      T.p(x + 1, y, c);
      T.p(x, y - 1, c);
      T.p(x, y + 1, c);
    });
    T.p(10, 2, 'S');
    T.p(23, 21, 'S');
  }, 'four-point rainbow sparkle');
  make('clock', 'Desk Clock', 'angle', T => {
    T.disc(16, 12, 10, 'R');
    T.disc(16, 12, 8, 'W');
    [[16, 5], [23, 12], [16, 19], [9, 12]].forEach(([x, y]) => T.p(x, y, 'P'));
    T.line(16, 12, 16, 7, 'K');
    T.line(16, 12, 20, 14, 'K');
    T.line(16, 12, 12, 8, 'A');
    T.line(16, 12, 21, 9, 'a');
    T.p(16, 12, 'P');
    T.outline(['R']);
  }, 'rainbow-rimmed clock; second hand ticks');
  make('connect', 'File Share', 'row', T => {
    [[1, 'R'], [20, 'E']].forEach(([x0, c], i) => {
      T.rect(x0, 6, x0 + 10, 16, 'H');
      T.hl(x0, 16, x0 + 10, 'h');
      T.rect(x0 + 1, 7, x0 + 9, 14, 'N');
      if (i === 0) T.rect(x0 + 2, 9, x0 + 8, 12, 'R');else {
        T.rect(x0 + 3, 8, x0 + 6, 13, 'W');
        T.hl(x0 + 4, 10, x0 + 5, 'w');
        T.hl(x0 + 4, 11, x0 + 5, 'w');
      }
    });
    T.outline(['H', 'h']);
    T.hl(13, 9, 17, 'A');
    T.p(16, 8, 'A');
    T.p(16, 10, 'A');
    T.hl(14, 13, 18, 'a');
    T.p(15, 12, 'a');
    T.p(15, 14, 'a');
    T.p(3, 2, 'S');
    T.p(28, 21, 'S');
  }, 'two Cheap Yellow Displays swapping a file');
  make('config', 'Settings', 'angle', T => {
    T.disc(16, 12, 7, 'w');
    T.ring(16, 12, 4, 5, 'R');
    T.disc(16, 12, 2, '.');
    T.p(12, 8, 'W');
    T.p(13, 7, 'W');
    T.p(11, 9, 'W');
    T.outline(['w']);
    for (let k = 0; k < 8; k++) {
      const a = k * 45 * Math.PI / 180;
      for (let d = 7; d <= 10; d++) for (let s = -1.2; s <= 1.2; s += 0.4) T.p(16 + Math.cos(a) * d - Math.sin(a) * s, 12 + Math.sin(a) * d + Math.cos(a) * s, 'w');
    }
    T.ring(16, 12, 4, 5, 'R');
    T.p(12, 8, 'W');
    T.p(13, 7, 'W');
    T.p(11, 9, 'W');
    T.outline(['w']);
    for (let k = 0; k < 8; k++) {
      const a = (k * 45 + 22.5) * Math.PI / 180;
      T.p(16 + Math.cos(a) * 12, 12 + Math.sin(a) * 12, k % 2 ? 'a' : 'A');
    }
    T.disc(16, 12, 2, 'K', (x, y) => Math.hypot(x - 16, y - 12) > 1.5);
    T.disc(16, 12, 1, '.');
  }, 'gear with a rainbow hub; sparks orbit');
  return {
    W,
    H,
    PALETTE,
    icons
  };
}
if (typeof module !== 'undefined') module.exports = {
  buildIcons
};
})(); } catch (e) { __ds_ns.__errors.push({ path: "icons/build_icons.js", error: String((e && e.message) || e) }); }

// icons/generate.js
try { (() => {
// Regenerates the icon outputs from build_icons.js. Run inside the design tool's
// script runner (readFile/saveFile/createCanvas), or port to Node in a few lines.
async function generateIcons({
  readFile,
  saveFile,
  createCanvas
}, opts = {}) {
  const src = await readFile('icons/build_icons.js');
  const buildIcons = new Function(src.replace('if (typeof module', 'if (false && typeof module') + ';return buildIcons;')();
  const {
    W,
    H,
    PALETTE,
    icons
  } = buildIcons();
  const keys = PALETTE.map(p => p[0]);
  const ids = Object.keys(icons);
  const rgb565 = ([r, g, b]) => (r & 0xf8) << 8 | (g & 0xfc) << 3 | b >> 3;

  // 1. JS data module for the React components.
  let js = '// Generated by icons/generate.js from icons/build_icons.js. Do not edit.\n';
  js += `export const ICON_W = ${W};\nexport const ICON_H = ${H};\n`;
  js += 'export const PALETTE = ' + JSON.stringify(PALETTE.map(([k, v, d]) => ({
    k,
    v,
    d
  }))) + ';\n';
  js += 'export const ICONS = {\n' + ids.map(id => {
    const i = icons[id];
    return `  ${id}: { id: '${id}', name: ${JSON.stringify(i.name)}, mode: '${i.mode}', note: ${JSON.stringify(i.note)}, grid: [\n${i.grid.map(r => `    '${r}',`).join('\n')}\n  ] },`;
  }).join('\n') + '\n};\n';
  js += `export const ICON_ORDER = ${JSON.stringify(ids)};\n`;
  await saveFile('components/icons/iconData.js', js);

  // 2. C header for the firmware (same index scheme as unicorn_sprite.h).
  const modeC = {
    row: 'CYI_ROW',
    col: 'CYI_COL',
    diag: 'CYI_DIAG',
    angle: 'CYI_ANGLE'
  };
  let h = '// Generated by icons/generate.js from icons/build_icons.js. Do not edit.\n';
  h += '// CyberUnicorn 5000 app icons: 32x24 palette-indexed pixel art.\n';
  h += '// Indices 0-9 match unicorn_sprite.h. Draw with cyber_icon_draw.h.\n#pragma once\n#include <stdint.h>\n\n';
  h += `#define CYI_W ${W}\n#define CYI_H ${H}\n#define CYI_COUNT ${ids.length}\n`;
  h += '#define CYI_TRANSPARENT 0\n#define CYI_RAINBOW 1\n';
  h += `#define CYI_SPARKLE ${keys.indexOf('S')}\n#define CYI_FRAME_A ${keys.indexOf('A')}\n#define CYI_FRAME_B ${keys.indexOf('a')}\n#define CYI_FLOOR ${keys.indexOf('N')}\n#define CYI_GRID_A ${keys.indexOf('Q')}\n#define CYI_GRID_B ${keys.indexOf('q')}\n\n`;
  h += 'enum CyberIconMode : uint8_t { CYI_ROW, CYI_COL, CYI_DIAG, CYI_ANGLE };\n\n';
  h += '// RGB565 per index; rainbow, sparkle and rainbow frame A/B are 0 here (computed).\n';
  h += `static const uint16_t cyi_palette[${PALETTE.length}] = {\n`;
  h += PALETTE.map(([k, v, d]) => `    0x${(Array.isArray(v) ? rgb565(v) : v === 'gridA' || v === 'gridB' ? rgb565([0, 160, 255]) : 0).toString(16).toUpperCase().padStart(4, '0')}, // ${k} ${d}`).join('\n') + '\n};\n\n';
  for (const id of ids) {
    h += `static const uint8_t cyi_${id}[CYI_W * CYI_H] = {\n`;
    h += icons[id].grid.map(r => '    ' + [...r].map(c => keys.indexOf(c)).join(',') + ',').join('\n') + '\n};\n';
  }
  h += '\nstruct CyberIcon {\n    const char *id;\n    const char *name;\n    uint8_t mode;\n    const uint8_t *px;\n};\n\n';
  h += 'static const CyberIcon cyi_icons[CYI_COUNT] = {\n';
  h += ids.map(id => `    {"${id}", "${icons[id].name}", ${modeC[icons[id].mode]}, cyi_${id}},`).join('\n') + '\n};\n';
  await saveFile('icons/cyber_icons.h', h);

  // 3. Preview sheet (frame A and B), 4 device px per cell.
  const hue = h0 => {
    const x = (h0 % 360 + 360) % 360,
      s = Math.floor(x / 60),
      f = x % 60 * 255 / 60;
    return [[255, f, 0], [255 - f, 255, 0], [0, 255, f], [0, 255 - f, 255], [f, 0, 255], [255, 0, 255 - f]][s];
  };
  const base = (m, x, y) => m === 'row' ? y * 18 : m === 'col' ? x * 12 : m === 'diag' ? (x + y) * 10 : Math.atan2(y - 11.5, x - 15.5) * 180 / Math.PI + 180;
  const pal = Object.fromEntries(PALETTE.map(([k, v]) => [k, v]));
  const S = 4,
    cw = W * S + 16,
    ch = H * S + 16,
    cols = 6,
    rows = Math.ceil(ids.length / cols);
  const cv = createCanvas(cw * cols, ch * rows * 2),
    c = cv.getContext('2d');
  c.fillStyle = '#000';
  c.fillRect(0, 0, cv.width, cv.height);
  for (let fr = 0; fr < 2; fr++) ids.forEach((id, i) => {
    const ic = icons[id],
      ox = i % cols * cw + 8,
      oy = (Math.floor(i / cols) + fr * rows) * ch + 8,
      t = fr * 6;
    ic.grid.forEach((row, y) => [...row].forEach((k, x) => {
      let v = pal[k];
      if (v === null) return;
      if (v === 'gridA') v = fr ? [12, 0, 28] : [0, 160, 255];
      if (v === 'gridB') v = fr ? [0, 160, 255] : [12, 0, 28];
      if (v === 'frameA' && fr) return;
      if (v === 'frameB' && !fr) return;
      if (v === 'rainbow' || v === 'frameA' || v === 'frameB') v = hue(base(ic.mode, x, y) + t * 12);
      if (v === 'sparkle') {
        const p = (x * 3 + y * 5 + fr) % 6;
        if (p > 2) return;
        v = [[255, 255, 255], [0, 255, 255], [255, 90, 200]][p];
      }
      c.fillStyle = `rgb(${v.map(Math.round).join(',')})`;
      c.fillRect(ox + x * S, oy + y * S, S, S);
    }));
  });
  await saveFile(opts.preview || 'icons/icons_preview.png', cv);
  if (opts.zoom) {
    const z = opts.zoom,
      list = opts.zoomIds || ids,
      zc = createCanvas((W * z + 20) * list.length, (H * z + 20) * 2),
      zx = zc.getContext('2d');
    zx.fillStyle = '#000';
    zx.fillRect(0, 0, zc.width, zc.height);
    for (let fr = 0; fr < 2; fr++) list.forEach((id, i) => {
      const ic = icons[id],
        t = fr * 6;
      ic.grid.forEach((row, y) => [...row].forEach((k, x) => {
        let v = pal[k];
        if (v === null) return;
        if (v === 'gridA') v = fr ? [12, 0, 28] : [0, 160, 255];
        if (v === 'gridB') v = fr ? [0, 160, 255] : [12, 0, 28];
        if (v === 'frameA' && fr) return;
        if (v === 'frameB' && !fr) return;
        if (v === 'rainbow' || v === 'frameA' || v === 'frameB') v = hue(base(ic.mode, x, y) + t * 12);
        if (v === 'sparkle') {
          const p = (x * 3 + y * 5 + fr) % 6;
          if (p > 2) return;
          v = [[255, 255, 255], [0, 255, 255], [255, 90, 200]][p];
        }
        zx.fillStyle = `rgb(${v.map(Math.round).join(',')})`;
        zx.fillRect(10 + i * (W * z + 20) + x * z, 10 + fr * (H * z + 20) + y * z, z, z);
      }));
    });
    await saveFile(opts.zoomPath, zc);
  }
  return ids;
}
if (typeof module !== 'undefined') module.exports = {
  generateIcons
};
})(); } catch (e) { __ds_ns.__errors.push({ path: "icons/generate.js", error: String((e && e.message) || e) }); }

// ui_kits/device/AppScreen.jsx
try { (() => {
// App launch screen: title bar in rainbow, the icon at 2x, what the app does.
// Each app's real screens are firmware-specific; this is the shared launch frame.
const APP_LINES = {
  frequency: ['Write venue check-in tags', 'Read a tag', 'Scan member card', 'Write log'],
  lightshow: ['18 scenes', 'Brightness 40% max', 'Play / stop', 'Data pin 27'],
  sensors: ['Wired: Grove I2C', 'Wireless: ESP-NOW nodes', 'Altimeter', 'CSV log every 10 s'],
  led: ['Colour picker', 'Rainbow cycle', 'Breathe', 'Off'],
  light: ['LDR on GPIO34', 'Big %', 'Min / max', 'History'],
  sound: ['Test tone', 'Unicorn jingle', 'Sweep 100 Hz-8 kHz', 'Volume 1-5'],
  slideshow: ['Plays /slides', 'JPG PNG BMP GIF', 'NEXT / PREV', '320x240 max'],
  weather: ['Open-Meteo', '3-day strip', 'Indoor page', 'Metric / imperial'],
  hat: ['Probe live module', 'DIP 1 CC1101', 'DIP 3 PN532', 'Only one of 1-5 ON'],
  gpio: ['Read 22 27 35', 'Toggle', 'PWM', 'Logic trace'],
  remote: ['USB serial', 'Bluetooth now', 'Bluetooth at boot', 'Recent commands']
};
function AppScreen({
  app,
  onEsc
}) {
  const {
    PixelIcon,
    RainbowText,
    RainbowFrame,
    ICONS
  } = window.CyberUnicorn5000DesignSystem_f09c19;
  const icon = ICONS[app];
  const lines = APP_LINES[app] || [icon.note];
  const [sel, setSel] = React.useState(0);
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0,
      background: '#000',
      fontFamily: 'var(--font-display)'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      height: 22,
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      padding: '0 8px',
      borderBottom: '1px solid var(--cu-magenta)'
    }
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 1,
    animate: true
  }, icon.name.toUpperCase()), /*#__PURE__*/React.createElement("button", {
    onClick: onEsc,
    style: {
      all: 'unset',
      cursor: 'pointer',
      fontSize: 6,
      color: 'var(--cu-cyan)'
    }
  }, "ESC")), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 10,
      padding: 10,
      alignItems: 'center'
    }
  }, /*#__PURE__*/React.createElement(PixelIcon, {
    name: app,
    cell: 3
  }), /*#__PURE__*/React.createElement(RainbowFrame, {
    thickness: 1,
    animate: true,
    padding: 6,
    style: {
      flex: 1
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      flexDirection: 'column',
      gap: 2
    }
  }, lines.map((l, i) => /*#__PURE__*/React.createElement("button", {
    key: l,
    onClick: () => setSel(i),
    style: {
      all: 'unset',
      cursor: 'pointer',
      fontSize: 6,
      lineHeight: '14px',
      padding: '0 4px',
      color: i === sel ? '#000' : '#fff',
      background: i === sel ? 'var(--cu-magenta)' : 'transparent'
    }
  }, l))))), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 10,
      right: 10,
      bottom: 8,
      fontSize: 6,
      lineHeight: '9px',
      color: 'var(--cu-lavender)'
    }
  }, icon.note));
}
window.AppScreen = AppScreen;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/device/AppScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/device/apps/AppsA.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
// Light Show, Sensor Hub, RGB LED, Light Meter, Sound & Tones.
// Options, titles and footers are the strings in each app's .cpp.

// scenes.cpp display order: 11 mystic, then 7 classic.
const SCENES = ['Singing Bowl', 'Soft Rain', 'Prana Breath', 'Seed of Life', 'Lotus Bloom', 'Golden Spiral', 'Metatron', 'Chakra', 'Third Eye', 'Data Stream', 'Synthwave', 'Rainbow', 'Plasma', 'Fire', 'Sparkle', 'Rain', 'Breathe', 'Text'];
const SCENE_HUE = [190, 210, 160, 50, 320, 45, 270, 0, 260, 140, 300, 0, 0, 20, 0, 200, 300, 0];
// One panel pixel for scene s at brightness lvl (1-4), tick t.
function sceneColor(s, lvl, t, x, y) {
  const rainbow = [7, 10, 11, 12, 14].includes(s),
    base = rainbow ? null : cyberHue(SCENE_HUE[s]);
  const d = Math.hypot(x - 3.5, y - 3.5),
    w = (Math.sin(d * 1.3 - t / 3) + 1) / 2;
  const c = base ? base.map(v => v * (0.15 + 0.85 * w)) : cyberHue((s === 7 ? y * 51 : (x + y) * 30) + t * 8);
  return q565(c.map(v => Math.round(v * (0.4 + lvl * 0.15))));
}
function useShow(nav, back, start = 10) {
  const [s, setS] = React.useState(start),
    [lvl, setLvl] = React.useState(3),
    [shown, setShown] = React.useState(0),
    t = useTicker(60);
  useNav(nav, k => {
    if (k === 'next') setS(v => (v + 1) % 18);
    if (k === 'prev') setS(v => (v + 17) % 18);
    if (k === 'sel') setLvl(v => v % 4 + 1);
    if (k === 'esc') back();
    setShown(t);
  });
  return {
    s,
    lvl,
    t,
    fresh: t - shown < 34
  };
}
function LightShowRun({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText,
    RainbowText
  } = CU;
  const {
    s,
    lvl,
    t
  } = useShow(nav, back);
  const name = SCENES[s];
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "LIGHT SHOW"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 22,
      top: 50,
      display: 'grid',
      gridTemplateColumns: 'repeat(8, 15px)',
      gap: 2,
      padding: 4,
      background: '#1e0046'
    }
  }, Array.from({
    length: 64
  }, (_, i) => /*#__PURE__*/React.createElement("div", {
    key: i,
    style: {
      width: 15,
      height: 15,
      borderRadius: 3,
      background: sceneColor(s, lvl, t, i % 8, i >> 3)
    }
  }))), /*#__PURE__*/React.createElement(RainbowText, {
    size: 2,
    animate: true,
    style: {
      position: 'absolute',
      left: 178,
      top: 56
    }
  }, name.toUpperCase().split(' ').slice(0, 2).join(' ').slice(0, 11)), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 80,
    color: LGREY
  }, `${s + 1} of 18`), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 96,
    color: GREY
  }, s < 11 ? 'mystic' : 'classic'), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 112
  }, "level"), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 178,
      top: 124,
      display: 'flex',
      gap: 3
    }
  }, [1, 2, 3, 4].map(n => /*#__PURE__*/React.createElement("div", {
    key: n,
    style: {
      width: 22,
      height: 8,
      background: n <= lvl ? hue(300 + n * 30) : '#301040'
    }
  }))), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 140,
    color: GREY
  }, `${lvl * 10}% (max 40)`), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 156,
    color: GREY
  }, "GPIO 27  screen only"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 172,
    color: GREY
  }, "NEXT/PREV scene"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 182,
    color: GREY
  }, "SEL       bright"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 178,
    y: 192,
    color: GREY
  }, "ESC       exit"));
}
// Fullscreen: the 8x8 panel at 30 px per LED fills the screen's height, glow
// bleeding into the black side bars; scene name flashes for ~2 s after a key.
function LightShowFull({
  nav,
  back
}) {
  const {
    ScreenText,
    RainbowText
  } = CU;
  const {
    s,
    lvl,
    t,
    fresh
  } = useShow(nav, back);
  const cells = Array.from({
    length: 64
  }, (_, i) => sceneColor(s, lvl, t, i % 8, i >> 3));
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0,
      background: '#000',
      overflow: 'hidden'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0,
      filter: 'blur(18px)',
      opacity: 0.55,
      display: 'grid',
      gridTemplateColumns: 'repeat(8, 1fr)',
      gridTemplateRows: 'repeat(8, 1fr)'
    }
  }, cells.map((c, i) => /*#__PURE__*/React.createElement("div", {
    key: i,
    style: {
      background: c
    }
  }))), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 40,
      top: 0,
      width: 240,
      height: 240,
      display: 'grid',
      gridTemplateColumns: 'repeat(8, 30px)',
      gridTemplateRows: 'repeat(8, 30px)'
    }
  }, cells.map((c, i) => /*#__PURE__*/React.createElement("div", {
    key: i,
    style: {
      margin: 2,
      borderRadius: 6,
      background: c,
      boxShadow: `inset 0 0 0 2px rgba(255,255,255,.12)`
    }
  }))), fresh && /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 0,
      right: 0,
      bottom: 0,
      height: 34,
      background: 'rgba(0,0,0,.78)'
    }
  }), /*#__PURE__*/React.createElement(RainbowText, {
    size: 2,
    animate: true,
    style: {
      position: 'absolute',
      left: 8,
      top: 214
    }
  }, SCENES[s].toUpperCase()), /*#__PURE__*/React.createElement(ScreenText, {
    x: 312,
    y: 218,
    align: "right",
    color: LGREY
  }, `${s + 1}/18  ${lvl * 10}%`)));
}
function LightShowApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "LIGHT SHOW",
    icon: "lightshow",
    options: ['Start show', 'Fullscreen show', {
      label: 'Data pin',
      value: '27'
    }, {
      label: 'Zigzag rows',
      value: 'on'
    }, {
      label: 'Rotate',
      value: '0'
    }],
    screens: [LightShowRun, LightShowFull]
  }));
}
function SensorsLive({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText
  } = CU;
  const t = useTicker(400);
  useNav(nav, k => k === 'esc' && back());
  const rows = [['Temp', `${(21.4 + Math.sin(t / 6) * .3).toFixed(1)} C`, 'BME280', 'via I2C', 30], ['Humidity', `${(48 + Math.sin(t / 9) * 2).toFixed(0)} %`, 'BME280', 'via I2C', 200], ['Pressure', '1013.2 hPa', 'BME280', 'via I2C', 45], ['Light', `${(312 + Math.sin(t / 4) * 20).toFixed(0)} lux`, 'BH1750', 'via I2C', 52], ['Temp', '17.9 C', 'garden-01', 'via ESP-NOW', 160]];
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "SENSORS",
    footer: "NEXT detail    ESC back"
  }, /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 48,
    color: hue(160)
  }, "4 wired"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 82,
    y: 48,
    color: hue(200)
  }, "  ESP-NOW ch 1"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 250,
    y: 48,
    color: hue(0)
  }, "  * REC"), rows.map((r, i) => {
    const y = 64 + i * 26;
    const pts = Array.from({
      length: 24
    }, (_, j) => 8 - Math.round(Math.sin((j + t + i * 5) / 3) * 4 + Math.sin(j / 2) * 2));
    return /*#__PURE__*/React.createElement(React.Fragment, {
      key: i
    }, /*#__PURE__*/React.createElement(ScreenText, {
      x: 14,
      y: y,
      color: hue(r[4])
    }, r[0]), /*#__PURE__*/React.createElement(ScreenText, {
      x: 14,
      y: y + 10,
      color: GREY
    }, r[2] + ' ' + r[3]), /*#__PURE__*/React.createElement(ScreenText, {
      x: 210,
      y: y,
      size: 1,
      color: "#fff",
      align: "right",
      style: {
        right: 320 - 210
      }
    }, r[1]), /*#__PURE__*/React.createElement("svg", {
      width: "72",
      height: "18",
      style: {
        position: 'absolute',
        left: 232,
        top: y - 2
      }
    }, pts.map((v, j) => /*#__PURE__*/React.createElement("rect", {
      key: j,
      x: j * 3,
      y: v + 2,
      width: "3",
      height: "2",
      fill: hue(j * 15 + r[4])
    }))));
  }));
}
function SensorsAlt({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText,
    RainbowText
  } = CU;
  const t = useTicker(500);
  const [ft, setFt] = React.useState(false),
    [z, setZ] = React.useState(0);
  useNav(nav, k => {
    if (k === 'esc') back();
    if (k === 'next') setFt(v => !v);
    if (k === 'sel') setZ(t);
  });
  const m = (t - z) * 0.04 + Math.sin(t / 3) * 0.08;
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "ALTIMETER",
    footer: "SEL zero   NEXT m/ft   ESC back"
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 4,
    animate: true,
    style: {
      position: 'absolute',
      left: 24,
      top: 70
    }
  }, (ft ? m * 3.281 : m).toFixed(2)), /*#__PURE__*/React.createElement(ScreenText, {
    x: 150,
    y: 86,
    size: 2,
    color: hue(200)
  }, ft ? 'ft' : 'm'), /*#__PURE__*/React.createElement(Line, {
    y: 140,
    label: "Pressure",
    value: "1013.17 hPa   zero 1013.21",
    color: LGREY
  }), /*#__PURE__*/React.createElement(Line, {
    y: 152,
    label: "Source",
    value: "BME280 (I2C)",
    color: hue(160)
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 176,
    color: GREY
  }, "Relative to the zero point. RAM only."));
}
function SensorsApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "SENSORS",
    icon: "sensors",
    options: ['Live sensors', 'Altimeter', 'Scan I2C bus', {
      label: 'Logging',
      value: 'on'
    }, 'Node protocol'],
    screens: [SensorsLive, SensorsAlt]
  }));
}
const LED_PRESETS = [['Hot pink', 255, 20, 147], ['Cyber cyan', 0, 255, 255], ['Unicorn violet', 160, 0, 255], ['Toxic green', 80, 255, 0], ['Sunset orange', 255, 90, 0]];
function LedPicker({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText
  } = CU;
  const [h, setH] = React.useState(300),
    [lv, setLv] = React.useState(4);
  useNav(nav, k => {
    if (k === 'next') setH(v => (v + 15) % 360);
    if (k === 'prev') setH(v => (v + 345) % 360);
    if (k === 'sel') setLv(v => v % 5 + 1);
    if (k === 'esc') back();
  });
  const rgb = cyberHue(h).map(v => Math.round(v * lv / 5));
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "COLOUR",
    footer: "NEXT/PREV hue   SEL bright   ESC exit"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 24,
      top: 52,
      width: 110,
      height: 110,
      borderRadius: '50%',
      background: q565(rgb),
      boxShadow: `0 0 0 3px #000, 0 0 0 4px ${hue(h)}`
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 24,
      top: 170,
      width: 272,
      height: 8,
      background: `linear-gradient(90deg, ${Array.from({
        length: 13
      }, (_, i) => hue(i * 30)).join(',')})`
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 24 + h / 360 * 272 - 2,
      top: 166,
      width: 4,
      height: 16,
      background: '#fff'
    }
  }), ['R ', 'G ', 'B '].map((n, i) => /*#__PURE__*/React.createElement(React.Fragment, {
    key: n
  }, /*#__PURE__*/React.createElement(ScreenText, {
    x: 156,
    y: 60 + i * 18,
    size: 2,
    color: ['#ff3030', '#30ff60', '#4080ff'][i]
  }, n), /*#__PURE__*/React.createElement(ScreenText, {
    x: 186,
    y: 60 + i * 18,
    size: 2
  }, String(rgb[i]).padStart(3)))), /*#__PURE__*/React.createElement(ScreenText, {
    x: 156,
    y: 120,
    color: LGREY
  }, `${h} deg   level ${lv}`), /*#__PURE__*/React.createElement(ScreenText, {
    x: 156,
    y: 134,
    color: GREY
  }, "pins 4 / 16 / 17"));
}
function LedApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "LED",
    icon: "led",
    options: ['Colour picker', 'Rainbow cycle', 'Breathe', ...LED_PRESETS.map(x => x[0]), 'Off'],
    screens: [LedPicker]
  }));
}
function LightMeter({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText,
    RainbowText
  } = CU;
  const t = useTicker(120);
  useNav(nav, k => k === 'esc' && back());
  const pct = Math.round(58 + Math.sin(t / 10) * 14 + Math.sin(t / 3) * 3),
    band = pct < 10 ? 'midnight' : pct < 30 ? 'dim' : pct < 65 ? 'indoor' : pct < 90 ? 'bright' : 'SUNBEAM';
  const lc = v => hue(240 - v * 2.4 + 60);
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "LIGHT"
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 4,
    animate: true,
    style: {
      position: 'absolute',
      left: 14,
      top: 44
    }
  }, `${String(pct).padStart(3)}%`), /*#__PURE__*/React.createElement(ScreenText, {
    x: 126,
    y: 46,
    size: 2,
    color: lc(pct)
  }, band), /*#__PURE__*/React.createElement(ScreenText, {
    x: 126,
    y: 68
  }, `raw ${pct * 41}  ${pct * 33} mV`), /*#__PURE__*/React.createElement(ScreenText, {
    x: 126,
    y: 78
  }, `min 1640  max 3075`), /*#__PURE__*/React.createElement(ScreenText, {
    x: 126,
    y: 88,
    color: GREY
  }, "GPIO34 LDR"), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 12,
      top: 104,
      width: 296,
      height: 18,
      padding: 1,
      background: `linear-gradient(90deg, ${Array.from({
        length: 7
      }, (_, i) => hue(i * 60)).join(',')})`
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      height: 16,
      background: '#000',
      display: 'flex'
    }
  }, Array.from({
    length: 74
  }, (_, i) => /*#__PURE__*/React.createElement("div", {
    key: i,
    style: {
      flex: 1,
      background: i < pct * 74 / 100 ? lc(i * 100 / 74) : '#101010'
    }
  })))), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 12,
      top: 130,
      width: 296,
      height: 64,
      padding: 1,
      background: `linear-gradient(90deg, ${Array.from({
        length: 7
      }, (_, i) => hue(180 + i * 60)).join(',')})`
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      height: 62,
      background: '#000',
      display: 'flex',
      alignItems: 'flex-end',
      gap: 1
    }
  }, Array.from({
    length: 98
  }, (_, i) => {
    const v = Math.round(58 + Math.sin((t - 98 + i) / 10) * 14 + Math.sin((t - 98 + i) / 3) * 3);
    return /*#__PURE__*/React.createElement("div", {
      key: i,
      style: {
        width: 2,
        height: v * 0.6,
        background: lc(v)
      }
    });
  }))), /*#__PURE__*/React.createElement(ScreenText, {
    x: 12,
    y: 200,
    color: GREY
  }, "SEL reset min/max    ESC exit"));
}
function LightApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "LIGHT",
    icon: "light",
    options: ['Light meter'],
    screens: [LightMeter]
  }));
}
function SoundTone({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText,
    RainbowText
  } = CU;
  const [hz, setHz] = React.useState(440),
    [on, setOn] = React.useState(true),
    t = useTicker(50);
  useNav(nav, k => {
    if (k === 'next') setHz(v => Math.min(8000, Math.round(v * 1.12)));
    if (k === 'prev') setHz(v => Math.max(100, Math.round(v / 1.12)));
    if (k === 'sel') setOn(v => !v);
    if (k === 'esc') back();
  });
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "TEST TONE",
    footer: 'NEXT/PREV pitch    SEL play/stop\nESC stop'
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 4,
    animate: true,
    style: {
      position: 'absolute',
      left: 0,
      right: 0,
      top: 54,
      textAlign: 'center'
    }
  }, `${hz} Hz`), /*#__PURE__*/React.createElement(ScreenText, {
    y: 92,
    align: "center",
    size: 2,
    color: on ? hue(120) : GREY
  }, on ? 'PLAYING' : 'stopped'), /*#__PURE__*/React.createElement("svg", {
    width: "296",
    height: "70",
    style: {
      position: 'absolute',
      left: 12,
      top: 116
    }
  }, on && Array.from({
    length: 148
  }, (_, i) => {
    const f = j => Math.sin(j * hz / 2400 + t / 2) > 0 ? 1 : -1,
      sq = f(i),
      edge = i > 0 && f(i - 1) !== sq;
    return /*#__PURE__*/React.createElement(React.Fragment, {
      key: i
    }, /*#__PURE__*/React.createElement("rect", {
      x: i * 2,
      y: 35 - sq * 22,
      width: "2",
      height: "2",
      fill: hue(i * 2 + t * 6)
    }), edge && /*#__PURE__*/React.createElement("rect", {
      x: i * 2,
      y: 13,
      width: "2",
      height: "46",
      fill: hue(i * 2 + t * 6)
    }));
  }), !on && /*#__PURE__*/React.createElement("rect", {
    x: "0",
    y: "35",
    width: "296",
    height: "1",
    fill: "#333"
  })), /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 192,
    color: GREY
  }, "Speaker pin 26   volume 3/5"));
}
function SoundApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "SOUND",
    icon: "sound",
    options: ['Test tone', 'Unicorn jingle', 'Frequency sweep', 'Alerts', {
      label: 'Volume',
      value: '3/5'
    }],
    screens: [SoundTone]
  }));
}
Object.assign(window, {
  LightShowApp,
  SensorsApp,
  LedApp,
  LightApp,
  SoundApp
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/device/apps/AppsA.jsx", error: String((e && e.message) || e) }); }

// ui_kits/device/apps/AppsB.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
// Photo Slideshow, Weather Forecast, Hat Module Probe, GPIO Pins, Phone Remote.

function SlideshowRun({
  nav,
  back
}) {
  const {
    ScreenText
  } = CU;
  const [n, setN] = React.useState(2),
    t = useTicker(1000);
  const slides = [['boot_preview.png', '../../assets/boot_preview.png'], ['unicorn.png', '../../assets/unicorn_preview.png'], ['sunset.gif', null]];
  useNav(nav, k => {
    if (k === 'next' || k === 'sel') setN(v => (v + 1) % 10);
    if (k === 'prev') setN(v => (v + 9) % 10);
    if (k === 'esc') back();
  });
  React.useEffect(() => {
    if (t && t % 5 === 0) setN(v => (v + 1) % 10);
  }, [t]);
  const s = slides[n % 3];
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0,
      background: '#000'
    }
  }, s[1] ? /*#__PURE__*/React.createElement("img", {
    src: s[1],
    style: {
      position: 'absolute',
      left: '50%',
      top: '50%',
      transform: 'translate(-50%,-50%)',
      maxWidth: 320,
      maxHeight: 240,
      imageRendering: 'pixelated'
    }
  }) : /*#__PURE__*/React.createElement(Synthwave, null), /*#__PURE__*/React.createElement(ScreenText, {
    x: 4,
    y: 230,
    color: "#fff",
    bg: "#000"
  }, `${n + 1}/10  ${s[0]}`));
}
function SlideshowEmpty({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText,
    PixelIcon,
    RainbowText
  } = CU;
  useNav(nav, () => back());
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "SLIDESHOW",
    footer: "any key: back"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      top: 54,
      left: 0,
      right: 0,
      display: 'flex',
      justifyContent: 'center'
    }
  }, /*#__PURE__*/React.createElement(PixelIcon, {
    name: "slideshow",
    cell: 2
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      top: 112,
      left: 0,
      right: 0,
      display: 'flex',
      justifyContent: 'center'
    }
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 2,
    animate: true
  }, "No pictures yet")), /*#__PURE__*/React.createElement(ScreenText, {
    y: 140,
    align: "center"
  }, "Copy .jpg .png .bmp .gif files into"), /*#__PURE__*/React.createElement(ScreenText, {
    y: 152,
    align: "center",
    color: hue(180)
  }, "/slides on the SD card"), /*#__PURE__*/React.createElement(ScreenText, {
    y: 164,
    align: "center"
  }, "or /slides on LittleFS"), /*#__PURE__*/React.createElement(ScreenText, {
    y: 180,
    align: "center",
    color: GREY
  }, "Best fit: 320x240 or smaller"));
}
function SlideshowApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "SLIDESHOW",
    icon: "slideshow",
    options: ['Start slideshow', {
      label: 'Slide time',
      value: '5 s'
    }, {
      label: 'Loop',
      value: 'forever'
    }, 'Empty folder (preview)'],
    screens: [SlideshowRun, null, null, SlideshowEmpty]
  }));
}

// 12x12 condition sprites, verbatim from WeatherMenu.cpp.
const WX = {
  SUN: ['.....YY.....', '..Y..YY..Y..', '...Y....Y...', '....OOOO....', '...OYYYYO...', 'YY.OYYYYO.YY', 'YY.OYYYYO.YY', '...OYYYYO...', '....OOOO....', '...Y....Y...', '..Y..YY..Y..', '.....YY.....'],
  CLOUD: ['............', '............', '....WWW.....', '...WWWWW.WW.', '..WWWWWWWWWW', '.WWWWWWWWWWW', 'WWWWWWWWWWWW', 'WWWWWWWWWWWW', '.MMMMMMMMMM.', '............', '............', '............'],
  RAIN: ['..B..B..B...', '.B..B..B..B.', 'B..B..B..B..']
};
const WXC = {
  Y: hue(52),
  O: hue(28),
  W: '#fff',
  M: hue(300),
  B: hue(190)
};
function WxSprite({
  kind,
  x,
  y,
  s
}) {
  const cells = [];
  const put = (rows, ox, oy) => rows.forEach((r, j) => [...r].forEach((c, i) => c !== '.' && cells.push(/*#__PURE__*/React.createElement("div", {
    key: cells.length,
    style: {
      position: 'absolute',
      left: ox + i * s,
      top: oy + j * s,
      width: s,
      height: s,
      background: WXC[c]
    }
  }))));
  if (kind === 'sun') put(WX.SUN, x, y);
  if (kind === 'partly') {
    put(WX.SUN, x - 2 * s, y - 2 * s);
    put(WX.CLOUD, x + s, y + 2 * s);
  }
  if (kind === 'rain') {
    put(WX.CLOUD, x, y);
    put(WX.RAIN, x, y + 9 * s);
  }
  return /*#__PURE__*/React.createElement(React.Fragment, null, cells);
}
function Temp({
  t,
  x,
  y,
  size
}) {
  const {
    ScreenText
  } = CU;
  const c = hue(Math.max(0, 240 - (t + 5) * 7));
  const s = String(t);
  return /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement(ScreenText, {
    x: x,
    y: y,
    size: size,
    color: c
  }, s), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: x + s.length * 6 * size + 1,
      top: y,
      width: size * 2,
      height: size * 2,
      border: `1px solid ${c}`,
      borderRadius: '50%'
    }
  }));
}
function WeatherRun({
  nav,
  back
}) {
  const {
    ScreenText
  } = CU;
  const [indoor, setIndoor] = React.useState(false);
  useNav(nav, k => {
    if (k === 'esc') back();
    if (k === 'next') setIndoor(v => !v);
  });
  if (indoor) return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0
    }
  }, /*#__PURE__*/React.createElement(Synthwave, {
    sun: false
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 10,
    size: 2,
    color: hue(300)
  }, "INDOOR"), /*#__PURE__*/React.createElement(Temp, {
    t: 22,
    x: 10,
    y: 44,
    size: 5
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 96,
    size: 2,
    color: hue(200)
  }, "48%"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 70,
    y: 102,
    color: LGREY
  }, "HUMID"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 122,
    color: LGREY
  }, "PRESSURE  1013 hPa   BME280"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 226,
    color: LGREY
  }, "NEXT outdoor    ESC back"));
  const days = [['WED', 'rain', 14, 9], ['THU', 'partly', 17, 10], ['FRI', 'sun', 19, 11]];
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0
    }
  }, /*#__PURE__*/React.createElement(Synthwave, null), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 10,
    size: 2,
    color: hue(300)
  }, "PORTLAND"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 30,
    color: "#fff"
  }, "Partly cloudy   TODAY"), /*#__PURE__*/React.createElement(Temp, {
    t: 16,
    x: 10,
    y: 48,
    size: 5
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 94,
    color: LGREY
  }, "FEELS"), /*#__PURE__*/React.createElement(Temp, {
    t: 14,
    x: 50,
    y: 94,
    size: 1
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 84,
    y: 94,
    color: LGREY
  }, "HI"), /*#__PURE__*/React.createElement(Temp, {
    t: 18,
    x: 102,
    y: 94,
    size: 1
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 136,
    y: 94,
    color: LGREY
  }, "LO"), /*#__PURE__*/React.createElement(Temp, {
    t: 9,
    x: 154,
    y: 94,
    size: 1
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 106,
    color: hue(200)
  }, "HUMID 62%   WIND 11 km/h"), /*#__PURE__*/React.createElement(WxSprite, {
    kind: "partly",
    x: 214,
    y: 28,
    s: 6
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 6,
      right: 6,
      top: 164,
      height: 52,
      display: 'grid',
      gridTemplateColumns: 'repeat(3, 1fr)',
      gap: 4
    }
  }, days.map(([d, k, hi, lo]) => /*#__PURE__*/React.createElement("div", {
    key: d,
    style: {
      position: 'relative',
      background: 'rgba(0,0,0,.72)',
      border: `1px solid ${hue(300)}`
    }
  }, /*#__PURE__*/React.createElement(ScreenText, {
    x: 6,
    y: 6,
    color: "#fff"
  }, d), /*#__PURE__*/React.createElement(WxSprite, {
    kind: k,
    x: 8,
    y: 18,
    s: 2
  }), /*#__PURE__*/React.createElement(Temp, {
    t: hi,
    x: 46,
    y: 18,
    size: 2
  }), /*#__PURE__*/React.createElement(Temp, {
    t: lo,
    x: 50,
    y: 38,
    size: 1
  })))), /*#__PURE__*/React.createElement(ScreenText, {
    x: 10,
    y: 224,
    color: LGREY
  }, "SEL refresh   NEXT indoor"));
}
function WeatherApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "WEATHER",
    icon: "weather",
    options: ['Show weather', {
      label: 'Units',
      value: 'metric'
    }, {
      label: 'Location',
      value: 'auto (IP)'
    }],
    screens: [WeatherRun]
  }));
}
function HatDetect({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText
  } = CU;
  useNav(nav, () => back());
  const rows = [['PN532 NFC', 'live (I2C 0x24)', hue(120)], ['CC1101', 'no answer', GREY], ['nRF24', 'no answer', GREY], ['IR / 433', 'quiet', GREY]];
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "HAT MODULES",
    footer: "any key: back"
  }, rows.map((r, i) => /*#__PURE__*/React.createElement(Line, {
    key: i,
    y: 54 + i * 14,
    label: r[0],
    value: '  ' + r[1],
    color: r[2]
  })), /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 122,
    color: LGREY
  }, "DIP 3 is ON. Use RFID or Frequency."), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 60,
      top: 142,
      width: 200,
      height: 44,
      background: hue(300),
      display: 'flex',
      gap: 8,
      padding: '4px 10px',
      boxSizing: 'border-box'
    }
  }, [1, 2, 3, 4, 5, 6].map(n => /*#__PURE__*/React.createElement("div", {
    key: n,
    style: {
      flex: 1,
      background: '#0c001c',
      position: 'relative'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 1,
      right: 1,
      height: 16,
      top: n === 3 ? 1 : 19,
      background: '#fff'
    }
  })))), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 60,
      top: 188,
      width: 200,
      display: 'flex',
      justifyContent: 'space-around',
      fontFamily: "'Press Start 2P'",
      fontSize: 6,
      color: GREY
    }
  }, [1, 2, 3, 4, 5, 6].map(n => /*#__PURE__*/React.createElement("span", {
    key: n,
    style: {
      color: n === 3 ? hue(300) : GREY
    }
  }, n))));
}
function HatGuide({
  nav,
  back
}) {
  const {
    AppFrame
  } = CU;
  useNav(nav, () => back());
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "DIP SWITCH",
    footer: "any key: back"
  }, /*#__PURE__*/React.createElement(TextArea, {
    top: 50,
    lines: [{
      t: 'Only ONE of 1-5 ON at a time.',
      c: hue(300)
    }, 'They all share pins 22/27.', '', '1  CC1101 sub-GHz  -> RF', '2  nRF24 2.4 GHz   -> NRF24', '3  PN532 NFC       -> RFID', '4  IR TX/RX        -> IR', '5  433 MHz OOK     -> RF', '6  battery power', '', {
      t: 'I2C sensors on Grove can share',
      c: LGREY
    }, {
      t: 'the bus with 3 (PN532) only.',
      c: LGREY
    }]
  }));
}
function HatApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "HAT",
    icon: "hat",
    options: ['Detect modules', 'DIP switch guide'],
    screens: [HatDetect, HatGuide]
  }));
}
function GpioRead({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText,
    RainbowText
  } = CU;
  const t = useTicker(80);
  useNav(nav, k => k === 'esc' && back());
  const lvl = v => Math.sin(v / 5) + Math.sin(v / 13) > 0 ? 1 : 0,
    high = lvl(t);
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "GPIO 22",
    footer: "SEL reset count    ESC exit"
  }, /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 48,
    color: GREY
  }, "P3+CN1 / Grove SCL, hat CE"), /*#__PURE__*/React.createElement(RainbowText, {
    size: 4,
    animate: true,
    style: {
      position: 'absolute',
      left: 14,
      top: 66
    }
  }, high ? 'HIGH' : 'LOW'), /*#__PURE__*/React.createElement(ScreenText, {
    x: 150,
    y: 68,
    color: LGREY
  }, "read, pull-up"), /*#__PURE__*/React.createElement(ScreenText, {
    x: 150,
    y: 80
  }, `edges  ${Math.floor(t / 7)}`), /*#__PURE__*/React.createElement(ScreenText, {
    x: 150,
    y: 92
  }, "analog  ", high ? 3297 : 12, " mV"), /*#__PURE__*/React.createElement("svg", {
    width: "296",
    height: "70",
    style: {
      position: 'absolute',
      left: 12,
      top: 122
    }
  }, /*#__PURE__*/React.createElement("rect", {
    x: "0",
    y: "0",
    width: "296",
    height: "70",
    fill: "#0c001c",
    stroke: hue(300)
  }), Array.from({
    length: 148
  }, (_, i) => {
    const v = lvl(t - 148 + i),
      e = i > 0 && lvl(t - 149 + i) !== v;
    return /*#__PURE__*/React.createElement(React.Fragment, {
      key: i
    }, /*#__PURE__*/React.createElement("rect", {
      x: i * 2,
      y: v ? 12 : 54,
      width: "2",
      height: "3",
      fill: hue(i * 2 + t * 4)
    }), e && /*#__PURE__*/React.createElement("rect", {
      x: i * 2,
      y: 12,
      width: "2",
      height: "45",
      fill: hue(i * 2 + t * 4)
    }));
  })));
}
function GpioApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "GPIO",
    icon: "gpio",
    options: [{
      label: 'GPIO 22',
      value: 'SCL'
    }, {
      label: 'GPIO 27',
      value: 'SDA'
    }, {
      label: 'GPIO 35',
      value: 'input only'
    }],
    screens: [GpioRead, GpioRead, GpioRead]
  }));
}
function RemoteStatus({
  nav,
  back
}) {
  const {
    AppFrame,
    ScreenText
  } = CU;
  const [ble, setBle] = React.useState(false);
  useNav(nav, k => {
    if (k === 'sel') setBle(v => !v);
    if (k === 'esc') back();
  });
  const recent = [['system.status', true], ['display start', true], ['screen.nav', true], ['frequency.read_tag', false], ['led.set', true]];
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: "REMOTE",
    footer: "SEL: Bluetooth on/off   ESC: back",
    link: ble ? 'BLE' : 'USB'
  }, /*#__PURE__*/React.createElement(Line, {
    y: 50,
    label: "USB",
    value: "serial 115200, always on",
    color: hue(120)
  }), /*#__PURE__*/React.createElement(Line, {
    y: 62,
    label: "BLE",
    value: ble ? 'ON  CU5K  [boot]' : 'OFF  [boot]',
    color: ble ? hue(200) : GREY
  }), /*#__PURE__*/React.createElement(Line, {
    y: 74,
    label: "RAM",
    value: ble ? '41 KB free,  37 cmds' : '96 KB free,  37 cmds'
  }), /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 96,
    color: hue(300)
  }, "RECENT"), recent.map(([c, ok], i) => /*#__PURE__*/React.createElement(React.Fragment, {
    key: i
  }, /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 110 + i * 12,
    color: ok ? '#fff' : LGREY
  }, c), /*#__PURE__*/React.createElement(ScreenText, {
    x: 300,
    y: 110 + i * 12,
    align: "right",
    color: ok ? hue(120) : hue(0)
  }, ok ? 'ok' : 'no_reader'))), /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: 176,
    color: GREY
  }, "Phone app: Frequency Experience"));
}
function RemoteApp(p) {
  return /*#__PURE__*/React.createElement(MenuApp, _extends({}, p, {
    title: "REMOTE",
    icon: "remote",
    options: ['Status', {
      label: 'Bluetooth now',
      value: 'OFF'
    }, {
      label: 'Bluetooth at boot',
      value: 'OFF'
    }, 'How to connect'],
    screens: [RemoteStatus]
  }));
}
Object.assign(window, {
  SlideshowApp,
  WeatherApp,
  HatApp,
  GpioApp,
  RemoteApp
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/device/apps/AppsB.jsx", error: String((e && e.message) || e) }); }

// ui_kits/device/apps/Frequency.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
// FREQUENCY CHECK-IN: venue check-in tags on the PN532. Menu and copy from
// FrequencyMenu.cpp. Full-colour treatment: every screen sits on the synthwave
// scene, content in glass panels, sparkles everywhere, confetti on success.
const FREQ_OPTIONS = ['Program venue tag', 'Verify tag', 'Member scan', 'Issue member card', 'Tags written', 'Settings'];
const FREQ_SPACES = [['Moonlight Yoga', 'https://frequencylocal.com/n/moonlight-yoga?s=v1.Hq3'], ['Riverside Coffee', 'https://frequencylocal.com/n/riverside?s=v1.k9P'], ['Type a URL...', ''], ['Paste over serial (help)', '']];
const FREQ_GO = ['pick', 'verify', 'dip', 'dip', 'log', 'menu'];
const GLASS = 'rgba(12,0,28,.86)';
function FreqScene({
  sun = true,
  horizon = 150,
  sunX = 160,
  sunR = 44,
  sparkles = 30,
  burst,
  sparkTop = 48
}) {
  return /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement(Synthwave, {
    sun: sun,
    horizon: horizon,
    sunX: sunX,
    sunR: sunR
  }), /*#__PURE__*/React.createElement(Sparkles, {
    count: sparkles,
    top: sparkTop,
    bottom: horizon - 4,
    burst: burst
  }));
}
function FrequencyApp({
  nav,
  onExit,
  initial = 'menu'
}) {
  const {
    AppFrame,
    OptionList,
    ScreenText,
    PixelIcon,
    RainbowText
  } = CU;
  const [screen, setScreen] = React.useState(initial);
  const [space, setSpace] = React.useState(0);
  const [i, setI] = useMenu(nav, FREQ_OPTIONS.length, idx => setScreen(FREQ_GO[idx]), onExit, screen === 'menu');
  const [p, setP] = useMenu(nav, FREQ_SPACES.length, idx => {
    setSpace(idx);
    setScreen('tap');
  }, () => setScreen('menu'), screen === 'pick');
  useNav(nav, k => k === 'esc' ? setScreen('menu') : setScreen(screen === 'tap' ? 'written' : 'read'), ['tap', 'verify'].includes(screen));
  useNav(nav, k => setScreen(k === 'esc' || screen !== 'written' ? 'menu' : 'tap'), ['written', 'read', 'dip', 'log'].includes(screen));
  React.useEffect(() => {
    if (!nav || screen !== 'tap' && screen !== 'verify') return;
    const id = setTimeout(() => setScreen(screen === 'tap' ? 'written' : 'read'), 3200);
    return () => clearTimeout(id);
  }, [screen]);
  const url = FREQ_SPACES[space][1] || FREQ_SPACES[0][1];
  const name = FREQ_SPACES[space][1] ? FREQ_SPACES[space][0] : 'Moonlight Yoga';
  const F = {
    footerColor: LGREY,
    background: '#000'
  };
  const Centre = ({
    y,
    children
  }) => /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      top: y,
      left: 0,
      right: 0,
      display: 'flex',
      justifyContent: 'center'
    }
  }, children);
  if (screen === 'menu') return /*#__PURE__*/React.createElement(AppFrame, _extends({
    title: "FREQUENCY"
  }, F, {
    backdrop: /*#__PURE__*/React.createElement(FreqScene, {
      horizon: 132,
      sunR: 40
    })
  }), /*#__PURE__*/React.createElement(Centre, {
    y: 48
  }, /*#__PURE__*/React.createElement(PixelIcon, {
    name: "frequency",
    cell: 3
  })), /*#__PURE__*/React.createElement(OptionList, {
    options: FREQ_OPTIONS,
    selected: i,
    onSelect: idx => {
      setI(idx);
      setScreen(FREQ_GO[idx]);
    },
    top: 126,
    maxRows: 5,
    background: GLASS
  }));
  if (screen === 'pick') return /*#__PURE__*/React.createElement(AppFrame, _extends({
    title: "PROGRAM TAG",
    footer: "SEL write    ESC back"
  }, F, {
    backdrop: /*#__PURE__*/React.createElement(FreqScene, {
      sunX: 262,
      sunR: 30,
      horizon: 150,
      sparkTop: 92
    })
  }), /*#__PURE__*/React.createElement(OptionList, {
    title: "Check-in or event URL",
    options: FREQ_SPACES.map(s => s[0]),
    selected: p,
    onSelect: idx => {
      setSpace(idx);
      setScreen('tap');
    },
    top: 70,
    background: GLASS
  }), /*#__PURE__*/React.createElement(Panel, {
    x: 60,
    y: 164,
    w: 200,
    h: 18
  }, /*#__PURE__*/React.createElement(ScreenText, {
    y: 5,
    align: "center",
    width: 198,
    color: hue(180)
  }, "From /frequency/tags.txt")));
  if (screen === 'tap' || screen === 'verify') return /*#__PURE__*/React.createElement(AppFrame, _extends({
    title: screen === 'tap' ? 'PROGRAM TAG' : 'VERIFY TAG',
    footer: "ESC: back"
  }, F, {
    backdrop: /*#__PURE__*/React.createElement(FreqScene, {
      sun: false,
      horizon: 176,
      sparkles: 40,
      sparkTop: 92
    })
  }), /*#__PURE__*/React.createElement(Centre, {
    y: 52
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 2,
    animate: true
  }, screen === 'tap' ? 'TAP A TAG' : 'TAP ANY TAG')), /*#__PURE__*/React.createElement(Panel, {
    x: 14,
    y: 70,
    w: 292,
    h: 16
  }, /*#__PURE__*/React.createElement(ScreenText, {
    y: 4,
    align: "center",
    width: 290,
    color: "#fff"
  }, screen === 'tap' ? url.replace('https://', '').slice(0, 46) : 'shows where a tag points')), /*#__PURE__*/React.createElement(Wave, {
    x: 12,
    y: 90,
    w: 296,
    h: 108,
    clear: true
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 136,
      top: 120,
      filter: 'drop-shadow(0 0 6px #ff00c8)'
    }
  }, /*#__PURE__*/React.createElement(PixelIcon, {
    name: "frequency",
    cell: 1
  })));
  if (screen === 'written' || screen === 'read') return /*#__PURE__*/React.createElement(AppFrame, _extends({
    title: screen === 'written' ? 'TAG WRITTEN' : 'VERIFY TAG',
    footer: screen === 'written' ? 'any key: next' : 'any key: back'
  }, F, {
    backdrop: /*#__PURE__*/React.createElement(FreqScene, {
      sunX: 268,
      sunR: 34,
      horizon: 196,
      sparkles: 34,
      burst: screen === 'written' ? [268, 74] : null
    })
  }), /*#__PURE__*/React.createElement(Centre, {
    y: 50
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 2,
    animate: true
  }, screen === 'written' ? 'NAILED IT' : 'VENUE TAG')), /*#__PURE__*/React.createElement(Panel, {
    x: 8,
    y: 72,
    w: 226,
    h: 118
  }, /*#__PURE__*/React.createElement(Line, {
    y: 8,
    label: "Tag",
    value: "NTAG215"
  }), /*#__PURE__*/React.createElement(Line, {
    y: 20,
    label: "UID",
    value: "04:A1:7C:22:9B:51",
    color: LGREY
  }), /*#__PURE__*/React.createElement(Line, {
    y: 32,
    label: "Points to",
    value: "frequencylocal.com",
    color: hue(180)
  }), /*#__PURE__*/React.createElement(Line, {
    y: 44,
    label: "Space",
    value: name,
    color: hue(300)
  }), /*#__PURE__*/React.createElement(Line, {
    y: 56,
    label: "Signed",
    value: "yes (?s=v1...)",
    color: hue(120)
  }), /*#__PURE__*/React.createElement(Line, {
    y: 68,
    label: screen === 'written' ? 'Verified' : 'NFC mark',
    value: screen === 'written' ? 'read back OK' : '?m=nfc ok',
    color: hue(120)
  }), /*#__PURE__*/React.createElement(Line, {
    y: 80,
    label: "Size",
    value: "98 bytes NDEF",
    color: hue(52)
  }), /*#__PURE__*/React.createElement(Line, {
    y: 96,
    label: "",
    value: screen === 'written' ? 'Tap it with a phone' : 'Opens the Space check-in',
    color: "#fff"
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 240,
      top: 112
    }
  }, /*#__PURE__*/React.createElement(PixelIcon, {
    name: "frequency",
    cell: 2
  })));
  if (screen === 'dip') return /*#__PURE__*/React.createElement(AppFrame, _extends({
    title: "FREQUENCY",
    footer: "any key: back"
  }, F, {
    backdrop: /*#__PURE__*/React.createElement(FreqScene, {
      sunX: 160,
      sunR: 46,
      horizon: 200,
      sparkles: 24
    })
  }), /*#__PURE__*/React.createElement(Centre, {
    y: 54
  }, /*#__PURE__*/React.createElement(RainbowText, {
    size: 2,
    animate: true
  }, "TURN DIP 3 ON")), /*#__PURE__*/React.createElement(Panel, {
    x: 58,
    y: 74,
    w: 204,
    h: 76
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 10,
      top: 8,
      display: 'flex',
      gap: 8
    }
  }, [1, 2, 3, 4, 5, 6].map(n => {
    const on = n === 3,
      c = on ? hue(300) : '#7b7b7b';
    return /*#__PURE__*/React.createElement("div", {
      key: n,
      style: {
        width: 24,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        gap: 5
      }
    }, /*#__PURE__*/React.createElement("div", {
      style: {
        width: 24,
        height: 46,
        border: `1px solid ${c}`,
        boxSizing: 'border-box',
        position: 'relative',
        background: on ? 'rgba(255,0,200,.15)' : 'transparent',
        boxShadow: on ? `0 0 8px ${c}` : 'none'
      }
    }, /*#__PURE__*/React.createElement("div", {
      style: {
        position: 'absolute',
        left: 2,
        right: 2,
        height: 20,
        top: on ? 2 : 22,
        background: on ? `linear-gradient(${hue(300)}, ${hue(330)})` : c
      }
    })), /*#__PURE__*/React.createElement(ScreenText, {
      color: on ? '#fff' : c,
      style: {
        position: 'static'
      }
    }, String(n)));
  }))), /*#__PURE__*/React.createElement(Panel, {
    x: 22,
    y: 156,
    w: 276,
    h: 30
  }, /*#__PURE__*/React.createElement(ScreenText, {
    y: 5,
    align: "center",
    width: 274
  }, "The PN532 (NFC) is not answering."), /*#__PURE__*/React.createElement(ScreenText, {
    y: 17,
    align: "center",
    width: 274,
    color: hue(52)
  }, "Only one of DIP 1-5 may be ON.")));
  return /*#__PURE__*/React.createElement(AppFrame, _extends({
    title: "TAGS WRITTEN",
    footer: "ESC: back"
  }, F, {
    backdrop: /*#__PURE__*/React.createElement(FreqScene, {
      sunX: 270,
      sunR: 28,
      horizon: 204,
      sparkles: 20
    })
  }), /*#__PURE__*/React.createElement(Panel, {
    x: 10,
    y: 50,
    w: 300,
    h: 146
  }, /*#__PURE__*/React.createElement(TextArea, {
    top: 8,
    lines: [{
      t: 'Moonlight Yoga  [locked]',
      c: hue(300)
    }, {
      t: '  frequencylocal.com/n/moonlight-yoga',
      c: hue(180)
    }, {
      t: '  NTAG215  2026-10-20 16:12',
      c: LGREY
    }, '', {
      t: 'Riverside Coffee',
      c: hue(300)
    }, {
      t: '  frequencylocal.com/n/riverside',
      c: hue(180)
    }, {
      t: '  NTAG213  2026-10-19 11:40',
      c: LGREY
    }, '', {
      t: 'Log: /frequency/written.csv',
      c: hue(52)
    }]
  })));
}
window.FrequencyApp = FrequencyApp;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/device/apps/Frequency.jsx", error: String((e && e.message) || e) }); }

// ui_kits/device/apps/shared.jsx
try { (() => {
// Shared bits for the app screens: D-pad routing and the fx helpers the
// firmware uses (fxWave rainbow wave, synthwave backdrop).
const CU = window.CyberUnicorn5000DesignSystem_f09c19;
const GREY = '#7b7b7b',
  LGREY = '#c6c6c6';
const cyberHue = h0 => {
  let h = h0 % 360;
  if (h < 0) h += 360;
  const s = Math.floor(h / 60),
    f = Math.floor(h % 60 * 255 / 60);
  return [[255, f, 0], [255 - f, 255, 0], [0, 255, f], [0, 255 - f, 255], [f, 0, 255], [255, 0, 255 - f]][s];
};
const q565 = ([r, g, b]) => `rgb(${r & 0xf8 | r >> 5},${g & 0xfc | g >> 6},${b & 0xf8 | b >> 5})`;
const hue = h => q565(cyberHue(h));

// nav = {k:'prev'|'next'|'sel'|'esc', n}. Calls handler once per press.
// Inactive hooks still record presses, so a screen never replays the key that opened it.
function useNav(nav, handler, active = true) {
  const ref = React.useRef(handler);
  ref.current = handler;
  const act = React.useRef(active);
  act.current = active;
  const seen = React.useRef(nav && nav.n);
  React.useEffect(() => {
    if (nav && nav.n !== seen.current) {
      seen.current = nav.n;
      if (act.current) ref.current(nav.k);
    }
  }, [nav]);
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
  React.useEffect(() => {
    const id = setInterval(() => setT(v => v + 1), ms);
    return () => clearInterval(id);
  }, [ms]);
  return t;
}

// fxWave(): rainbow sine ribbons while waiting for a tag.
function Wave({
  x = 12,
  y = 88,
  w = 296,
  h = 110,
  clear = false
}) {
  const ref = React.useRef(null),
    t = useTicker();
  React.useEffect(() => {
    const c = ref.current.getContext('2d');
    if (clear) c.clearRect(0, 0, w, h);else {
      c.fillStyle = '#000';
      c.fillRect(0, 0, w, h);
    }
    for (let b = 0; b < 6; b++) for (let i = 0; i < w; i++) {
      const yy = h / 2 + Math.sin((i + t * 4) / 18 + b * 0.5) * (h / 2 - 8) * Math.sin((i + t * 2) / 60) + (b - 2.5) * 3;
      c.fillStyle = hue(b * 60 - t * 8 + i);
      c.fillRect(i, Math.round(yy), 1, 3);
    }
  }, [t]);
  return /*#__PURE__*/React.createElement("canvas", {
    ref: ref,
    width: w,
    height: h,
    style: {
      position: 'absolute',
      left: x,
      top: y,
      imageRendering: 'pixelated'
    }
  });
}

// drawBackdrop() from WeatherMenu.cpp: sky gradient, striped sun, magenta grid.
function Synthwave({
  sun = true,
  horizon = 146,
  sunX = 268,
  sunR = 30
}) {
  const ref = React.useRef(null);
  React.useEffect(() => {
    const c = ref.current.getContext('2d'),
      W = 320,
      H = 240,
      hy = horizon;
    for (let y = 0; y < hy; y++) {
      const k = y * 255 / hy;
      c.fillStyle = `rgb(${10 + k * 90 / 255 | 0},${k * 10 / 255 | 0},${40 + k * 60 / 255 | 0})`;
      c.fillRect(0, y, W, 1);
    }
    if (sun) {
      const cx = sunX,
        r = sunR;
      for (let dy = -r; dy < 0; dy++) {
        const band = -dy;
        if (band < r * 0.47 && band % 5 < 5 - (band / (r / 10) | 0)) continue;
        const half = Math.sqrt(r * r - dy * dy) | 0,
          k = (dy + r) * 255 / r;
        c.fillStyle = `rgb(255,${220 - k * 180 / 255 | 0},${60 + k * 120 / 255 | 0})`;
        c.fillRect(cx - half, hy + dy, 2 * half, 1);
      }
    }
    c.fillStyle = '#000';
    c.fillRect(0, hy, W, H - hy);
    for (let x = 0; x < W; x++) {
      c.fillStyle = hue(x * 360 / W);
      c.fillRect(x, hy, 1, 1);
    }
    c.fillStyle = hue(300);
    for (let i = 1, y = hy + 3; y < H; i++, y += 2 + i * 2) c.fillRect(0, y, W, 1);
    c.strokeStyle = hue(300);
    for (let i = -8; i <= 8; i++) {
      c.beginPath();
      c.moveTo(W / 2 + i * 12 + .5, hy);
      c.lineTo(W / 2 + i * 60 + .5, H);
      c.stroke();
    }
  }, [sun, horizon, sunX, sunR]);
  return /*#__PURE__*/React.createElement("canvas", {
    ref: ref,
    width: 320,
    height: 240,
    style: {
      position: 'absolute',
      inset: 0
    }
  });
}

// Twinkling 4-point sparkles over a screen; density = star count. burst = confetti
// shooting out from a point (for success screens).
function Sparkles({
  count = 26,
  seed = 7,
  burst,
  top = 0,
  bottom = 240
}) {
  const ref = React.useRef(null),
    t = useTicker(70);
  React.useEffect(() => {
    const c = ref.current.getContext('2d');
    c.clearRect(0, 0, 320, 240);
    const C = ['#ffffff', q565([0, 255, 255]), q565([255, 90, 200]), q565([255, 220, 60]), q565([0, 255, 160])];
    const star = (x, y, r, col) => {
      c.fillStyle = col;
      c.fillRect(x - r, y, 2 * r + 1, 1);
      c.fillRect(x, y - r, 1, 2 * r + 1);
      if (r > 2) {
        c.fillStyle = '#fff';
        c.fillRect(x - 1, y - 1, 3, 3);
      }
    };
    for (let i = 0; i < count; i++) {
      const x = (i * 97 + seed * 31) % 316 + 2,
        y = top + (i * 53 + seed * 17) % (bottom - top),
        ph = (t + i * 5) % 18;
      const r = ph < 3 ? ph : ph < 6 ? 6 - ph : 0;
      if (r) star(x, y, r + (i % 3 === 0 ? 1 : 0), C[i % 5]);
    }
    if (burst) for (let i = 0; i < 36; i++) {
      const a = i * 10 * Math.PI / 180,
        d = (t * 4 + i * 7) % 90 + 6,
        x = burst[0] + Math.cos(a) * d | 0,
        y = burst[1] + Math.sin(a) * d * 0.7 | 0;
      c.fillStyle = hue(i * 40 + t * 12);
      c.fillRect(x, y, 3, 3);
      if (i % 4 === 0) star(x, y, 2, '#fff');
    }
  }, [t]);
  return /*#__PURE__*/React.createElement("canvas", {
    ref: ref,
    width: 320,
    height: 240,
    style: {
      position: 'absolute',
      inset: 0,
      pointerEvents: 'none'
    }
  });
}
// Dark glass panel with a rainbow hairline, for content over a backdrop.
function Panel({
  x,
  y,
  w,
  h,
  children
}) {
  const stops = Array.from({
    length: 7
  }, (_, i) => hue(300 + i * 60)).join(',');
  const ring = 'linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0)';
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: x,
      top: y,
      width: w,
      height: h,
      borderRadius: 4,
      background: 'rgba(12,0,28,.86)'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0,
      padding: 1,
      borderRadius: 4,
      background: `linear-gradient(135deg, ${stops})`,
      WebkitMask: ring,
      mask: ring
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 1
    }
  }, children));
}
// line(): grey label at x 14, coloured value at x 82 (FrequencyMenu.cpp).
function Line({
  y,
  label,
  value,
  color = '#fff'
}) {
  const {
    ScreenText
  } = CU;
  return /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement(ScreenText, {
    x: 14,
    y: y,
    color: GREY
  }, label), /*#__PURE__*/React.createElement(ScreenText, {
    x: 82,
    y: y,
    color: color
  }, value));
}

// Bruce's scrolling text area: plain lines from y 46.
function TextArea({
  lines,
  top = 46,
  color = '#fff'
}) {
  const {
    ScreenText
  } = CU;
  return /*#__PURE__*/React.createElement(React.Fragment, null, lines.map((l, i) => /*#__PURE__*/React.createElement(ScreenText, {
    key: i,
    x: 14,
    y: top + i * 10,
    color: typeof l === 'object' ? l.c : color
  }, typeof l === 'object' ? l.t : l)));
}

// App top level: OptionList menu; SEL opens screens[i] (null = stays on menu).
// Screens mount fresh, so their own useNav never sees the key that opened them.
function MenuApp({
  nav,
  onExit,
  initial = 'menu',
  title,
  options,
  screens,
  icon
}) {
  const {
    AppFrame,
    OptionList,
    PixelIcon
  } = CU;
  const [screen, setScreen] = React.useState(initial);
  const open = idx => screens[idx] && setScreen(idx);
  const [i, setI] = useMenu(nav, options.length, open, onExit, screen === 'menu');
  if (screen !== 'menu') {
    const S = screens[screen];
    return /*#__PURE__*/React.createElement(S, {
      nav: nav,
      back: () => setScreen('menu')
    });
  }
  return /*#__PURE__*/React.createElement(AppFrame, {
    title: title
  }, icon && /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 0,
      right: 0,
      top: 48,
      display: 'flex',
      justifyContent: 'center'
    }
  }, /*#__PURE__*/React.createElement(PixelIcon, {
    name: icon,
    cell: 2
  })), /*#__PURE__*/React.createElement(OptionList, {
    options: options,
    selected: i,
    onSelect: idx => {
      setI(idx);
      open(idx);
    },
    top: icon ? 102 : 52,
    maxRows: icon ? 6 : 8
  }));
}
Object.assign(window, {
  CU,
  GREY,
  LGREY,
  hue,
  cyberHue,
  q565,
  useNav,
  useMenu,
  useTicker,
  Wave,
  Synthwave,
  Sparkles,
  Panel,
  Line,
  TextArea,
  MenuApp
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/device/apps/shared.jsx", error: String((e && e.message) || e) }); }

__ds_ns.StatusBar = __ds_scope.StatusBar;

__ds_ns.AppFrame = __ds_scope.AppFrame;

__ds_ns.BootScene = __ds_scope.BootScene;

__ds_ns.DeviceScreen = __ds_scope.DeviceScreen;

__ds_ns.MenuScreen = __ds_scope.MenuScreen;

__ds_ns.OptionList = __ds_scope.OptionList;

__ds_ns.ScreenText = __ds_scope.ScreenText;

__ds_ns.PixelIcon = __ds_scope.PixelIcon;

__ds_ns.UnicornSprite = __ds_scope.UnicornSprite;

__ds_ns.ICON_W = __ds_scope.ICON_W;

__ds_ns.ICON_H = __ds_scope.ICON_H;

__ds_ns.PALETTE = __ds_scope.PALETTE;

__ds_ns.ICONS = __ds_scope.ICONS;

__ds_ns.ICON_ORDER = __ds_scope.ICON_ORDER;

__ds_ns.UNICORN_PALETTE = __ds_scope.UNICORN_PALETTE;

__ds_ns.UNICORN_FRAMES = __ds_scope.UNICORN_FRAMES;

__ds_ns.RainbowBar = __ds_scope.RainbowBar;

__ds_ns.RainbowFrame = __ds_scope.RainbowFrame;

__ds_ns.RainbowText = __ds_scope.RainbowText;

})();
