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
  remote: ['USB serial', 'Bluetooth now', 'Bluetooth at boot', 'Recent commands'],
};
function AppScreen({ app, onEsc }) {
  const { PixelIcon, RainbowText, RainbowFrame, ICONS } = window.CyberUnicorn5000DesignSystem_f09c19;
  const icon = ICONS[app];
  const lines = APP_LINES[app] || [icon.note];
  const [sel, setSel] = React.useState(0);
  return (
    <div style={{ position: 'absolute', inset: 0, background: '#000', fontFamily: 'var(--font-display)' }}>
      <div style={{ height: 22, display: 'flex', alignItems: 'center', justifyContent: 'space-between', padding: '0 8px', borderBottom: '1px solid var(--cu-magenta)' }}>
        <RainbowText size={1} animate>{icon.name.toUpperCase()}</RainbowText>
        <button onClick={onEsc} style={{ all: 'unset', cursor: 'pointer', fontSize: 6, color: 'var(--cu-cyan)' }}>ESC</button>
      </div>
      <div style={{ display: 'flex', gap: 10, padding: 10, alignItems: 'center' }}>
        <PixelIcon name={app} cell={3} />
        <RainbowFrame thickness={1} animate padding={6} style={{ flex: 1 }}>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            {lines.map((l, i) => (
              <button key={l} onClick={() => setSel(i)} style={{ all: 'unset', cursor: 'pointer', fontSize: 6, lineHeight: '14px', padding: '0 4px', color: i === sel ? '#000' : '#fff', background: i === sel ? 'var(--cu-magenta)' : 'transparent' }}>{l}</button>
            ))}
          </div>
        </RainbowFrame>
      </div>
      <div style={{ position: 'absolute', left: 10, right: 10, bottom: 8, fontSize: 6, lineHeight: '9px', color: 'var(--cu-lavender)' }}>{icon.note}</div>
    </div>
  );
}
window.AppScreen = AppScreen;
