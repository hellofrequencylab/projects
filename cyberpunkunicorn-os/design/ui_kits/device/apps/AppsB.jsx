// Photo Slideshow, Weather Forecast, Hat Module Probe, GPIO Pins, Phone Remote.

function SlideshowRun({ nav, back }) {
  const { ScreenText } = CU; const [n, setN] = React.useState(2), t = useTicker(1000);
  const slides = [['boot_preview.png', '../../assets/boot_preview.png'], ['unicorn.png', '../../assets/unicorn_preview.png'], ['sunset.gif', null]];
  useNav(nav, k => { if (k === 'next' || k === 'sel') setN(v => (v + 1) % 10); if (k === 'prev') setN(v => (v + 9) % 10); if (k === 'esc') back(); });
  React.useEffect(() => { if (t && t % 5 === 0) setN(v => (v + 1) % 10); }, [t]);
  const s = slides[n % 3];
  return <div style={{ position: 'absolute', inset: 0, background: '#000' }}>
    {s[1] ? <img src={s[1]} style={{ position: 'absolute', left: '50%', top: '50%', transform: 'translate(-50%,-50%)', maxWidth: 320, maxHeight: 240, imageRendering: 'pixelated' }} /> : <Synthwave />}
    <ScreenText x={4} y={230} color="#fff" bg="#000">{`${n + 1}/10  ${s[0]}`}</ScreenText>
  </div>;
}
function SlideshowEmpty({ nav, back }) {
  const { AppFrame, ScreenText, PixelIcon, RainbowText } = CU; useNav(nav, () => back());
  return <AppFrame title="SLIDESHOW" footer="any key: back">
    <div style={{ position: 'absolute', top: 54, left: 0, right: 0, display: 'flex', justifyContent: 'center' }}><PixelIcon name="slideshow" cell={2} /></div>
    <div style={{ position: 'absolute', top: 112, left: 0, right: 0, display: 'flex', justifyContent: 'center' }}><RainbowText size={2} animate>No pictures yet</RainbowText></div>
    <ScreenText y={140} align="center">Copy .jpg .png .bmp .gif files into</ScreenText>
    <ScreenText y={152} align="center" color={hue(180)}>/slides on the SD card</ScreenText>
    <ScreenText y={164} align="center">or /slides on LittleFS</ScreenText>
    <ScreenText y={180} align="center" color={GREY}>Best fit: 320x240 or smaller</ScreenText>
  </AppFrame>;
}
function SlideshowApp(p) {
  return <MenuApp {...p} title="SLIDESHOW" icon="slideshow" options={['Start slideshow', { label: 'Slide time', value: '5 s' }, { label: 'Loop', value: 'forever' }, 'Empty folder (preview)']} screens={[SlideshowRun, null, null, SlideshowEmpty]} />;
}

// 12x12 condition sprites, verbatim from WeatherMenu.cpp.
const WX = {
  SUN: ['.....YY.....', '..Y..YY..Y..', '...Y....Y...', '....OOOO....', '...OYYYYO...', 'YY.OYYYYO.YY', 'YY.OYYYYO.YY', '...OYYYYO...', '....OOOO....', '...Y....Y...', '..Y..YY..Y..', '.....YY.....'],
  CLOUD: ['............', '............', '....WWW.....', '...WWWWW.WW.', '..WWWWWWWWWW', '.WWWWWWWWWWW', 'WWWWWWWWWWWW', 'WWWWWWWWWWWW', '.MMMMMMMMMM.', '............', '............', '............'],
  RAIN: ['..B..B..B...', '.B..B..B..B.', 'B..B..B..B..'],
};
const WXC = { Y: hue(52), O: hue(28), W: '#fff', M: hue(300), B: hue(190) };
function WxSprite({ kind, x, y, s }) {
  const cells = [];
  const put = (rows, ox, oy) => rows.forEach((r, j) => [...r].forEach((c, i) => c !== '.' && cells.push(<div key={cells.length} style={{ position: 'absolute', left: ox + i * s, top: oy + j * s, width: s, height: s, background: WXC[c] }}></div>)));
  if (kind === 'sun') put(WX.SUN, x, y);
  if (kind === 'partly') { put(WX.SUN, x - 2 * s, y - 2 * s); put(WX.CLOUD, x + s, y + 2 * s); }
  if (kind === 'rain') { put(WX.CLOUD, x, y); put(WX.RAIN, x, y + 9 * s); }
  return <>{cells}</>;
}
function Temp({ t, x, y, size }) {
  const { ScreenText } = CU; const c = hue(Math.max(0, 240 - (t + 5) * 7)); const s = String(t);
  return <><ScreenText x={x} y={y} size={size} color={c}>{s}</ScreenText><div style={{ position: 'absolute', left: x + s.length * 6 * size + 1, top: y, width: size * 2, height: size * 2, border: `1px solid ${c}`, borderRadius: '50%' }}></div></>;
}
function WeatherRun({ nav, back }) {
  const { ScreenText } = CU; const [indoor, setIndoor] = React.useState(false);
  useNav(nav, k => { if (k === 'esc') back(); if (k === 'next') setIndoor(v => !v); });
  if (indoor) return <div style={{ position: 'absolute', inset: 0 }}><Synthwave sun={false} />
    <ScreenText x={10} y={10} size={2} color={hue(300)}>INDOOR</ScreenText>
    <Temp t={22} x={10} y={44} size={5} />
    <ScreenText x={10} y={96} size={2} color={hue(200)}>48%</ScreenText><ScreenText x={70} y={102} color={LGREY}>HUMID</ScreenText>
    <ScreenText x={10} y={122} color={LGREY}>PRESSURE  1013 hPa   BME280</ScreenText>
    <ScreenText x={10} y={226} color={LGREY}>NEXT outdoor    ESC back</ScreenText>
  </div>;
  const days = [['WED', 'rain', 14, 9], ['THU', 'partly', 17, 10], ['FRI', 'sun', 19, 11]];
  return <div style={{ position: 'absolute', inset: 0 }}><Synthwave />
    <ScreenText x={10} y={10} size={2} color={hue(300)}>PORTLAND</ScreenText>
    <ScreenText x={10} y={30} color="#fff">Partly cloudy   TODAY</ScreenText>
    <Temp t={16} x={10} y={48} size={5} />
    <ScreenText x={10} y={94} color={LGREY}>FEELS</ScreenText><Temp t={14} x={50} y={94} size={1} />
    <ScreenText x={84} y={94} color={LGREY}>HI</ScreenText><Temp t={18} x={102} y={94} size={1} />
    <ScreenText x={136} y={94} color={LGREY}>LO</ScreenText><Temp t={9} x={154} y={94} size={1} />
    <ScreenText x={10} y={106} color={hue(200)}>HUMID 62%   WIND 11 km/h</ScreenText>
    <WxSprite kind="partly" x={214} y={28} s={6} />
    <div style={{ position: 'absolute', left: 6, right: 6, top: 164, height: 52, display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 4 }}>
      {days.map(([d, k, hi, lo]) => <div key={d} style={{ position: 'relative', background: 'rgba(0,0,0,.72)', border: `1px solid ${hue(300)}` }}>
        <ScreenText x={6} y={6} color="#fff">{d}</ScreenText><WxSprite kind={k} x={8} y={18} s={2} />
        <Temp t={hi} x={46} y={18} size={2} /><Temp t={lo} x={50} y={38} size={1} />
      </div>)}
    </div>
    <ScreenText x={10} y={224} color={LGREY}>SEL refresh   NEXT indoor</ScreenText>
  </div>;
}
function WeatherApp(p) {
  return <MenuApp {...p} title="WEATHER" icon="weather" options={['Show weather', { label: 'Units', value: 'metric' }, { label: 'Location', value: 'auto (IP)' }]} screens={[WeatherRun]} />;
}

function HatDetect({ nav, back }) {
  const { AppFrame, ScreenText } = CU; useNav(nav, () => back());
  const rows = [['PN532 NFC', 'live (I2C 0x24)', hue(120)], ['CC1101', 'no answer', GREY], ['nRF24', 'no answer', GREY], ['IR / 433', 'quiet', GREY]];
  return <AppFrame title="HAT MODULES" footer="any key: back">
    {rows.map((r, i) => <Line key={i} y={54 + i * 14} label={r[0]} value={'  ' + r[1]} color={r[2]} />)}
    <ScreenText x={14} y={122} color={LGREY}>DIP 3 is ON. Use RFID or Frequency.</ScreenText>
    <div style={{ position: 'absolute', left: 60, top: 142, width: 200, height: 44, background: hue(300), display: 'flex', gap: 8, padding: '4px 10px', boxSizing: 'border-box' }}>
      {[1, 2, 3, 4, 5, 6].map(n => <div key={n} style={{ flex: 1, background: '#0c001c', position: 'relative' }}><div style={{ position: 'absolute', left: 1, right: 1, height: 16, top: n === 3 ? 1 : 19, background: '#fff' }}></div></div>)}
    </div>
    <div style={{ position: 'absolute', left: 60, top: 188, width: 200, display: 'flex', justifyContent: 'space-around', fontFamily: "'Press Start 2P'", fontSize: 6, color: GREY }}>{[1, 2, 3, 4, 5, 6].map(n => <span key={n} style={{ color: n === 3 ? hue(300) : GREY }}>{n}</span>)}</div>
  </AppFrame>;
}
function HatGuide({ nav, back }) {
  const { AppFrame } = CU; useNav(nav, () => back());
  return <AppFrame title="DIP SWITCH" footer="any key: back"><TextArea top={50} lines={[{ t: 'Only ONE of 1-5 ON at a time.', c: hue(300) }, 'They all share pins 22/27.', '', '1  CC1101 sub-GHz  -> RF', '2  nRF24 2.4 GHz   -> NRF24', '3  PN532 NFC       -> RFID', '4  IR TX/RX        -> IR', '5  433 MHz OOK     -> RF', '6  battery power', '', { t: 'I2C sensors on Grove can share', c: LGREY }, { t: 'the bus with 3 (PN532) only.', c: LGREY }]} /></AppFrame>;
}
function HatApp(p) { return <MenuApp {...p} title="HAT" icon="hat" options={['Detect modules', 'DIP switch guide']} screens={[HatDetect, HatGuide]} />; }

function GpioRead({ nav, back }) {
  const { AppFrame, ScreenText, RainbowText } = CU; const t = useTicker(80);
  useNav(nav, k => k === 'esc' && back());
  const lvl = v => (Math.sin(v / 5) + Math.sin(v / 13)) > 0 ? 1 : 0, high = lvl(t);
  return <AppFrame title="GPIO 22" footer="SEL reset count    ESC exit">
    <ScreenText x={14} y={48} color={GREY}>P3+CN1 / Grove SCL, hat CE</ScreenText>
    <RainbowText size={4} animate style={{ position: 'absolute', left: 14, top: 66 }}>{high ? 'HIGH' : 'LOW'}</RainbowText>
    <ScreenText x={150} y={68} color={LGREY}>read, pull-up</ScreenText>
    <ScreenText x={150} y={80}>{`edges  ${Math.floor(t / 7)}`}</ScreenText>
    <ScreenText x={150} y={92}>analog  {high ? 3297 : 12} mV</ScreenText>
    <svg width="296" height="70" style={{ position: 'absolute', left: 12, top: 122 }}>
      <rect x="0" y="0" width="296" height="70" fill="#0c001c" stroke={hue(300)} />
      {Array.from({ length: 148 }, (_, i) => { const v = lvl(t - 148 + i), e = i > 0 && lvl(t - 149 + i) !== v; return <React.Fragment key={i}><rect x={i * 2} y={v ? 12 : 54} width="2" height="3" fill={hue(i * 2 + t * 4)} />{e && <rect x={i * 2} y={12} width="2" height="45" fill={hue(i * 2 + t * 4)} />}</React.Fragment>; })}
    </svg>
  </AppFrame>;
}
function GpioApp(p) {
  return <MenuApp {...p} title="GPIO" icon="gpio" options={[{ label: 'GPIO 22', value: 'SCL' }, { label: 'GPIO 27', value: 'SDA' }, { label: 'GPIO 35', value: 'input only' }]} screens={[GpioRead, GpioRead, GpioRead]} />;
}

function RemoteStatus({ nav, back }) {
  const { AppFrame, ScreenText } = CU; const [ble, setBle] = React.useState(false);
  useNav(nav, k => { if (k === 'sel') setBle(v => !v); if (k === 'esc') back(); });
  const recent = [['system.status', true], ['display start', true], ['screen.nav', true], ['frequency.read_tag', false], ['led.set', true]];
  return <AppFrame title="REMOTE" footer="SEL: Bluetooth on/off   ESC: back" link={ble ? 'BLE' : 'USB'}>
    <Line y={50} label="USB" value="serial 115200, always on" color={hue(120)} />
    <Line y={62} label="BLE" value={ble ? 'ON  CU5K  [boot]' : 'OFF  [boot]'} color={ble ? hue(200) : GREY} />
    <Line y={74} label="RAM" value={ble ? '41 KB free,  37 cmds' : '96 KB free,  37 cmds'} />
    <ScreenText x={14} y={96} color={hue(300)}>RECENT</ScreenText>
    {recent.map(([c, ok], i) => <React.Fragment key={i}><ScreenText x={14} y={110 + i * 12} color={ok ? '#fff' : LGREY}>{c}</ScreenText><ScreenText x={300} y={110 + i * 12} align="right" color={ok ? hue(120) : hue(0)}>{ok ? 'ok' : 'no_reader'}</ScreenText></React.Fragment>)}
    <ScreenText x={14} y={176} color={GREY}>Phone app: Frequency Experience</ScreenText>
  </AppFrame>;
}
function RemoteApp(p) { return <MenuApp {...p} title="REMOTE" icon="remote" options={['Status', { label: 'Bluetooth now', value: 'OFF' }, { label: 'Bluetooth at boot', value: 'OFF' }, 'How to connect']} screens={[RemoteStatus]} />; }

Object.assign(window, { SlideshowApp, WeatherApp, HatApp, GpioApp, RemoteApp });
