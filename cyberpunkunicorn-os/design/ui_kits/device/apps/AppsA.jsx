// Light Show, Sensor Hub, RGB LED, Light Meter, Sound & Tones.
// Options, titles and footers are the strings in each app's .cpp.

// scenes.cpp display order: 11 mystic, then 7 classic.
const SCENES = ['Singing Bowl', 'Soft Rain', 'Prana Breath', 'Seed of Life', 'Lotus Bloom', 'Golden Spiral', 'Metatron', 'Chakra', 'Third Eye', 'Data Stream', 'Synthwave', 'Rainbow', 'Plasma', 'Fire', 'Sparkle', 'Rain', 'Breathe', 'Text'];
const SCENE_HUE = [190, 210, 160, 50, 320, 45, 270, 0, 260, 140, 300, 0, 0, 20, 0, 200, 300, 0];
// One panel pixel for scene s at brightness lvl (1-4), tick t.
function sceneColor(s, lvl, t, x, y) {
  const rainbow = [7, 10, 11, 12, 14].includes(s), base = rainbow ? null : cyberHue(SCENE_HUE[s]);
  const d = Math.hypot(x - 3.5, y - 3.5), w = (Math.sin(d * 1.3 - t / 3) + 1) / 2;
  const c = base ? base.map(v => v * (0.15 + 0.85 * w)) : cyberHue((s === 7 ? y * 51 : (x + y) * 30) + t * 8);
  return q565(c.map(v => Math.round(v * (0.4 + lvl * 0.15))));
}
function useShow(nav, back, start = 10) {
  const [s, setS] = React.useState(start), [lvl, setLvl] = React.useState(3), [shown, setShown] = React.useState(0), t = useTicker(60);
  useNav(nav, k => { if (k === 'next') setS(v => (v + 1) % 18); if (k === 'prev') setS(v => (v + 17) % 18); if (k === 'sel') setLvl(v => v % 4 + 1); if (k === 'esc') back(); setShown(t); });
  return { s, lvl, t, fresh: t - shown < 34 };
}
function LightShowRun({ nav, back }) {
  const { AppFrame, ScreenText, RainbowText } = CU;
  const { s, lvl, t } = useShow(nav, back);
  const name = SCENES[s];
  return <AppFrame title="LIGHT SHOW">
    <div style={{ position: 'absolute', left: 22, top: 50, display: 'grid', gridTemplateColumns: 'repeat(8, 15px)', gap: 2, padding: 4, background: '#1e0046' }}>
      {Array.from({ length: 64 }, (_, i) => <div key={i} style={{ width: 15, height: 15, borderRadius: 3, background: sceneColor(s, lvl, t, i % 8, i >> 3) }}></div>)}
    </div>
    <RainbowText size={2} animate style={{ position: 'absolute', left: 178, top: 56 }}>{name.toUpperCase().split(' ').slice(0, 2).join(' ').slice(0, 11)}</RainbowText>
    <ScreenText x={178} y={80} color={LGREY}>{`${s + 1} of 18`}</ScreenText>
    <ScreenText x={178} y={96} color={GREY}>{s < 11 ? 'mystic' : 'classic'}</ScreenText>
    <ScreenText x={178} y={112}>level</ScreenText>
    <div style={{ position: 'absolute', left: 178, top: 124, display: 'flex', gap: 3 }}>{[1, 2, 3, 4].map(n => <div key={n} style={{ width: 22, height: 8, background: n <= lvl ? hue(300 + n * 30) : '#301040' }}></div>)}</div>
    <ScreenText x={178} y={140} color={GREY}>{`${lvl * 10}% (max 40)`}</ScreenText>
    <ScreenText x={178} y={156} color={GREY}>GPIO 27  screen only</ScreenText>
    <ScreenText x={178} y={172} color={GREY}>NEXT/PREV scene</ScreenText>
    <ScreenText x={178} y={182} color={GREY}>SEL       bright</ScreenText>
    <ScreenText x={178} y={192} color={GREY}>ESC       exit</ScreenText>
  </AppFrame>;
}
// Fullscreen: the 8x8 panel at 30 px per LED fills the screen's height, glow
// bleeding into the black side bars; scene name flashes for ~2 s after a key.
function LightShowFull({ nav, back }) {
  const { ScreenText, RainbowText } = CU;
  const { s, lvl, t, fresh } = useShow(nav, back);
  const cells = Array.from({ length: 64 }, (_, i) => sceneColor(s, lvl, t, i % 8, i >> 3));
  return <div style={{ position: 'absolute', inset: 0, background: '#000', overflow: 'hidden' }}>
    <div style={{ position: 'absolute', inset: 0, filter: 'blur(18px)', opacity: 0.55, display: 'grid', gridTemplateColumns: 'repeat(8, 1fr)', gridTemplateRows: 'repeat(8, 1fr)' }}>{cells.map((c, i) => <div key={i} style={{ background: c }}></div>)}</div>
    <div style={{ position: 'absolute', left: 40, top: 0, width: 240, height: 240, display: 'grid', gridTemplateColumns: 'repeat(8, 30px)', gridTemplateRows: 'repeat(8, 30px)' }}>
      {cells.map((c, i) => <div key={i} style={{ margin: 2, borderRadius: 6, background: c, boxShadow: `inset 0 0 0 2px rgba(255,255,255,.12)` }}></div>)}
    </div>
    {fresh && <><div style={{ position: 'absolute', left: 0, right: 0, bottom: 0, height: 34, background: 'rgba(0,0,0,.78)' }}></div>
      <RainbowText size={2} animate style={{ position: 'absolute', left: 8, top: 214 }}>{SCENES[s].toUpperCase()}</RainbowText>
      <ScreenText x={312} y={218} align="right" color={LGREY}>{`${s + 1}/18  ${lvl * 10}%`}</ScreenText></>}
  </div>;
}
function LightShowApp(p) {
  return <MenuApp {...p} title="LIGHT SHOW" icon="lightshow" options={['Start show', 'Fullscreen show', { label: 'Data pin', value: '27' }, { label: 'Zigzag rows', value: 'on' }, { label: 'Rotate', value: '0' }]} screens={[LightShowRun, LightShowFull]} />;
}

function SensorsLive({ nav, back }) {
  const { AppFrame, ScreenText } = CU; const t = useTicker(400);
  useNav(nav, k => k === 'esc' && back());
  const rows = [['Temp', `${(21.4 + Math.sin(t / 6) * .3).toFixed(1)} C`, 'BME280', 'via I2C', 30], ['Humidity', `${(48 + Math.sin(t / 9) * 2).toFixed(0)} %`, 'BME280', 'via I2C', 200], ['Pressure', '1013.2 hPa', 'BME280', 'via I2C', 45], ['Light', `${(312 + Math.sin(t / 4) * 20).toFixed(0)} lux`, 'BH1750', 'via I2C', 52], ['Temp', '17.9 C', 'garden-01', 'via ESP-NOW', 160]];
  return <AppFrame title="SENSORS" footer="NEXT detail    ESC back">
    <ScreenText x={14} y={48} color={hue(160)}>4 wired</ScreenText><ScreenText x={82} y={48} color={hue(200)}>  ESP-NOW ch 1</ScreenText><ScreenText x={250} y={48} color={hue(0)}>  * REC</ScreenText>
    {rows.map((r, i) => { const y = 64 + i * 26; const pts = Array.from({ length: 24 }, (_, j) => 8 - Math.round(Math.sin((j + t + i * 5) / 3) * 4 + Math.sin(j / 2) * 2)); return <React.Fragment key={i}>
      <ScreenText x={14} y={y} color={hue(r[4])}>{r[0]}</ScreenText><ScreenText x={14} y={y + 10} color={GREY}>{r[2] + ' ' + r[3]}</ScreenText>
      <ScreenText x={210} y={y} size={1} color="#fff" align="right" style={{ right: 320 - 210 }}>{r[1]}</ScreenText>
      <svg width="72" height="18" style={{ position: 'absolute', left: 232, top: y - 2 }}>{pts.map((v, j) => <rect key={j} x={j * 3} y={v + 2} width="3" height="2" fill={hue(j * 15 + r[4])} />)}</svg>
    </React.Fragment>; })}
  </AppFrame>;
}
function SensorsAlt({ nav, back }) {
  const { AppFrame, ScreenText, RainbowText } = CU; const t = useTicker(500); const [ft, setFt] = React.useState(false), [z, setZ] = React.useState(0);
  useNav(nav, k => { if (k === 'esc') back(); if (k === 'next') setFt(v => !v); if (k === 'sel') setZ(t); });
  const m = ((t - z) * 0.04 + Math.sin(t / 3) * 0.08);
  return <AppFrame title="ALTIMETER" footer="SEL zero   NEXT m/ft   ESC back">
    <RainbowText size={4} animate style={{ position: 'absolute', left: 24, top: 70 }}>{(ft ? m * 3.281 : m).toFixed(2)}</RainbowText>
    <ScreenText x={150} y={86} size={2} color={hue(200)}>{ft ? 'ft' : 'm'}</ScreenText>
    <Line y={140} label="Pressure" value="1013.17 hPa   zero 1013.21" color={LGREY} />
    <Line y={152} label="Source" value="BME280 (I2C)" color={hue(160)} />
    <ScreenText x={14} y={176} color={GREY}>Relative to the zero point. RAM only.</ScreenText>
  </AppFrame>;
}
function SensorsApp(p) {
  return <MenuApp {...p} title="SENSORS" icon="sensors" options={['Live sensors', 'Altimeter', 'Scan I2C bus', { label: 'Logging', value: 'on' }, 'Node protocol']} screens={[SensorsLive, SensorsAlt]} />;
}

const LED_PRESETS = [['Hot pink', 255, 20, 147], ['Cyber cyan', 0, 255, 255], ['Unicorn violet', 160, 0, 255], ['Toxic green', 80, 255, 0], ['Sunset orange', 255, 90, 0]];
function LedPicker({ nav, back }) {
  const { AppFrame, ScreenText } = CU; const [h, setH] = React.useState(300), [lv, setLv] = React.useState(4);
  useNav(nav, k => { if (k === 'next') setH(v => (v + 15) % 360); if (k === 'prev') setH(v => (v + 345) % 360); if (k === 'sel') setLv(v => v % 5 + 1); if (k === 'esc') back(); });
  const rgb = cyberHue(h).map(v => Math.round(v * lv / 5));
  return <AppFrame title="COLOUR" footer="NEXT/PREV hue   SEL bright   ESC exit">
    <div style={{ position: 'absolute', left: 24, top: 52, width: 110, height: 110, borderRadius: '50%', background: q565(rgb), boxShadow: `0 0 0 3px #000, 0 0 0 4px ${hue(h)}` }}></div>
    <div style={{ position: 'absolute', left: 24, top: 170, width: 272, height: 8, background: `linear-gradient(90deg, ${Array.from({ length: 13 }, (_, i) => hue(i * 30)).join(',')})` }}></div>
    <div style={{ position: 'absolute', left: 24 + h / 360 * 272 - 2, top: 166, width: 4, height: 16, background: '#fff' }}></div>
    {['R ', 'G ', 'B '].map((n, i) => <React.Fragment key={n}><ScreenText x={156} y={60 + i * 18} size={2} color={['#ff3030', '#30ff60', '#4080ff'][i]}>{n}</ScreenText><ScreenText x={186} y={60 + i * 18} size={2}>{String(rgb[i]).padStart(3)}</ScreenText></React.Fragment>)}
    <ScreenText x={156} y={120} color={LGREY}>{`${h} deg   level ${lv}`}</ScreenText>
    <ScreenText x={156} y={134} color={GREY}>pins 4 / 16 / 17</ScreenText>
  </AppFrame>;
}
function LedApp(p) {
  return <MenuApp {...p} title="LED" icon="led" options={['Colour picker', 'Rainbow cycle', 'Breathe', ...LED_PRESETS.map(x => x[0]), 'Off']} screens={[LedPicker]} />;
}

function LightMeter({ nav, back }) {
  const { AppFrame, ScreenText, RainbowText } = CU; const t = useTicker(120);
  useNav(nav, k => k === 'esc' && back());
  const pct = Math.round(58 + Math.sin(t / 10) * 14 + Math.sin(t / 3) * 3), band = pct < 10 ? 'midnight' : pct < 30 ? 'dim' : pct < 65 ? 'indoor' : pct < 90 ? 'bright' : 'SUNBEAM';
  const lc = v => hue(240 - v * 2.4 + 60);
  return <AppFrame title="LIGHT">
    <RainbowText size={4} animate style={{ position: 'absolute', left: 14, top: 44 }}>{`${String(pct).padStart(3)}%`}</RainbowText>
    <ScreenText x={126} y={46} size={2} color={lc(pct)}>{band}</ScreenText>
    <ScreenText x={126} y={68}>{`raw ${pct * 41}  ${pct * 33} mV`}</ScreenText>
    <ScreenText x={126} y={78}>{`min 1640  max 3075`}</ScreenText>
    <ScreenText x={126} y={88} color={GREY}>GPIO34 LDR</ScreenText>
    <div style={{ position: 'absolute', left: 12, top: 104, width: 296, height: 18, padding: 1, background: `linear-gradient(90deg, ${Array.from({ length: 7 }, (_, i) => hue(i * 60)).join(',')})` }}><div style={{ height: 16, background: '#000', display: 'flex' }}>{Array.from({ length: 74 }, (_, i) => <div key={i} style={{ flex: 1, background: i < pct * 74 / 100 ? lc(i * 100 / 74) : '#101010' }}></div>)}</div></div>
    <div style={{ position: 'absolute', left: 12, top: 130, width: 296, height: 64, padding: 1, background: `linear-gradient(90deg, ${Array.from({ length: 7 }, (_, i) => hue(180 + i * 60)).join(',')})` }}><div style={{ height: 62, background: '#000', display: 'flex', alignItems: 'flex-end', gap: 1 }}>{Array.from({ length: 98 }, (_, i) => { const v = Math.round(58 + Math.sin((t - 98 + i) / 10) * 14 + Math.sin((t - 98 + i) / 3) * 3); return <div key={i} style={{ width: 2, height: v * 0.6, background: lc(v) }}></div>; })}</div></div>
    <ScreenText x={12} y={200} color={GREY}>SEL reset min/max    ESC exit</ScreenText>
  </AppFrame>;
}
function LightApp(p) { return <MenuApp {...p} title="LIGHT" icon="light" options={['Light meter']} screens={[LightMeter]} />; }

function SoundTone({ nav, back }) {
  const { AppFrame, ScreenText, RainbowText } = CU; const [hz, setHz] = React.useState(440), [on, setOn] = React.useState(true), t = useTicker(50);
  useNav(nav, k => { if (k === 'next') setHz(v => Math.min(8000, Math.round(v * 1.12))); if (k === 'prev') setHz(v => Math.max(100, Math.round(v / 1.12))); if (k === 'sel') setOn(v => !v); if (k === 'esc') back(); });
  return <AppFrame title="TEST TONE" footer={'NEXT/PREV pitch    SEL play/stop\nESC stop'}>
    <RainbowText size={4} animate style={{ position: 'absolute', left: 0, right: 0, top: 54, textAlign: 'center' }}>{`${hz} Hz`}</RainbowText>
    <ScreenText y={92} align="center" size={2} color={on ? hue(120) : GREY}>{on ? 'PLAYING' : 'stopped'}</ScreenText>
    <svg width="296" height="70" style={{ position: 'absolute', left: 12, top: 116 }}>{on && Array.from({ length: 148 }, (_, i) => { const f = j => Math.sin(j * hz / 2400 + t / 2) > 0 ? 1 : -1, sq = f(i), edge = i > 0 && f(i - 1) !== sq; return <React.Fragment key={i}><rect x={i * 2} y={35 - sq * 22} width="2" height="2" fill={hue(i * 2 + t * 6)} />{edge && <rect x={i * 2} y={13} width="2" height="46" fill={hue(i * 2 + t * 6)} />}</React.Fragment>; })}{!on && <rect x="0" y="35" width="296" height="1" fill="#333" />}</svg>
    <ScreenText x={14} y={192} color={GREY}>Speaker pin 26   volume 3/5</ScreenText>
  </AppFrame>;
}
function SoundApp(p) {
  return <MenuApp {...p} title="SOUND" icon="sound" options={['Test tone', 'Unicorn jingle', 'Frequency sweep', 'Alerts', { label: 'Volume', value: '3/5' }]} screens={[SoundTone]} />;
}
Object.assign(window, { LightShowApp, SensorsApp, LedApp, LightApp, SoundApp });
