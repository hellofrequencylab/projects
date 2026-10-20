// FREQUENCY CHECK-IN: venue check-in tags on the PN532. Menu and copy from
// FrequencyMenu.cpp. Full-colour treatment: every screen sits on the synthwave
// scene, content in glass panels, sparkles everywhere, confetti on success.
const FREQ_OPTIONS = ['Program venue tag', 'Verify tag', 'Member scan', 'Issue member card', 'Tags written', 'Settings'];
const FREQ_SPACES = [['Moonlight Yoga', 'https://frequencylocal.com/n/moonlight-yoga?s=v1.Hq3'], ['Riverside Coffee', 'https://frequencylocal.com/n/riverside?s=v1.k9P'], ['Type a URL...', ''], ['Paste over serial (help)', '']];
const FREQ_GO = ['pick', 'verify', 'dip', 'dip', 'log', 'menu'];
const GLASS = 'rgba(12,0,28,.86)';

function FreqScene({ sun = true, horizon = 150, sunX = 160, sunR = 44, sparkles = 30, burst, sparkTop = 48 }) {
  return <><Synthwave sun={sun} horizon={horizon} sunX={sunX} sunR={sunR} /><Sparkles count={sparkles} top={sparkTop} bottom={horizon - 4} burst={burst} /></>;
}

function FrequencyApp({ nav, onExit, initial = 'menu' }) {
  const { AppFrame, OptionList, ScreenText, PixelIcon, RainbowText } = CU;
  const [screen, setScreen] = React.useState(initial);
  const [space, setSpace] = React.useState(0);
  const [i, setI] = useMenu(nav, FREQ_OPTIONS.length, idx => setScreen(FREQ_GO[idx]), onExit, screen === 'menu');
  const [p, setP] = useMenu(nav, FREQ_SPACES.length, idx => { setSpace(idx); setScreen('tap'); }, () => setScreen('menu'), screen === 'pick');
  useNav(nav, k => k === 'esc' ? setScreen('menu') : setScreen(screen === 'tap' ? 'written' : 'read'), ['tap', 'verify'].includes(screen));
  useNav(nav, k => setScreen(k === 'esc' || screen !== 'written' ? 'menu' : 'tap'), ['written', 'read', 'dip', 'log'].includes(screen));
  React.useEffect(() => {
    if (!nav || (screen !== 'tap' && screen !== 'verify')) return;
    const id = setTimeout(() => setScreen(screen === 'tap' ? 'written' : 'read'), 3200); return () => clearTimeout(id);
  }, [screen]);
  const url = FREQ_SPACES[space][1] || FREQ_SPACES[0][1];
  const name = FREQ_SPACES[space][1] ? FREQ_SPACES[space][0] : 'Moonlight Yoga';
  const F = { footerColor: LGREY, background: '#000' };
  const Centre = ({ y, children }) => <div style={{ position: 'absolute', top: y, left: 0, right: 0, display: 'flex', justifyContent: 'center' }}>{children}</div>;

  if (screen === 'menu') return <AppFrame title="FREQUENCY" {...F} backdrop={<FreqScene horizon={132} sunR={40} />}>
    <Centre y={48}><PixelIcon name="frequency" cell={3} /></Centre>
    <OptionList options={FREQ_OPTIONS} selected={i} onSelect={idx => { setI(idx); setScreen(FREQ_GO[idx]); }} top={126} maxRows={5} background={GLASS} />
  </AppFrame>;

  if (screen === 'pick') return <AppFrame title="PROGRAM TAG" footer="SEL write    ESC back" {...F} backdrop={<FreqScene sunX={262} sunR={30} horizon={150} sparkTop={92} />}>
    <OptionList title="Check-in or event URL" options={FREQ_SPACES.map(s => s[0])} selected={p} onSelect={idx => { setSpace(idx); setScreen('tap'); }} top={70} background={GLASS} />
    <Panel x={60} y={164} w={200} h={18}><ScreenText y={5} align="center" width={198} color={hue(180)}>From /frequency/tags.txt</ScreenText></Panel>
  </AppFrame>;

  if (screen === 'tap' || screen === 'verify') return <AppFrame title={screen === 'tap' ? 'PROGRAM TAG' : 'VERIFY TAG'} footer="ESC: back" {...F} backdrop={<FreqScene sun={false} horizon={176} sparkles={40} sparkTop={92} />}>
    <Centre y={52}><RainbowText size={2} animate>{screen === 'tap' ? 'TAP A TAG' : 'TAP ANY TAG'}</RainbowText></Centre>
    <Panel x={14} y={70} w={292} h={16}><ScreenText y={4} align="center" width={290} color="#fff">{screen === 'tap' ? url.replace('https://', '').slice(0, 46) : 'shows where a tag points'}</ScreenText></Panel>
    <Wave x={12} y={90} w={296} h={108} clear />
    <div style={{ position: 'absolute', left: 136, top: 120, filter: 'drop-shadow(0 0 6px #ff00c8)' }}><PixelIcon name="frequency" cell={1} /></div>
  </AppFrame>;

  if (screen === 'written' || screen === 'read') return <AppFrame title={screen === 'written' ? 'TAG WRITTEN' : 'VERIFY TAG'} footer={screen === 'written' ? 'any key: next' : 'any key: back'} {...F} backdrop={<FreqScene sunX={268} sunR={34} horizon={196} sparkles={34} burst={screen === 'written' ? [268, 74] : null} />}>
    <Centre y={50}><RainbowText size={2} animate>{screen === 'written' ? 'NAILED IT' : 'VENUE TAG'}</RainbowText></Centre>
    <Panel x={8} y={72} w={226} h={118}>
      <Line y={8} label="Tag" value="NTAG215" />
      <Line y={20} label="UID" value="04:A1:7C:22:9B:51" color={LGREY} />
      <Line y={32} label="Points to" value="frequencylocal.com" color={hue(180)} />
      <Line y={44} label="Space" value={name} color={hue(300)} />
      <Line y={56} label="Signed" value="yes (?s=v1...)" color={hue(120)} />
      <Line y={68} label={screen === 'written' ? 'Verified' : 'NFC mark'} value={screen === 'written' ? 'read back OK' : '?m=nfc ok'} color={hue(120)} />
      <Line y={80} label="Size" value="98 bytes NDEF" color={hue(52)} />
      <Line y={96} label="" value={screen === 'written' ? 'Tap it with a phone' : 'Opens the Space check-in'} color="#fff" />
    </Panel>
    <div style={{ position: 'absolute', left: 240, top: 112 }}><PixelIcon name="frequency" cell={2} /></div>
  </AppFrame>;

  if (screen === 'dip') return <AppFrame title="FREQUENCY" footer="any key: back" {...F} backdrop={<FreqScene sunX={160} sunR={46} horizon={200} sparkles={24} />}>
    <Centre y={54}><RainbowText size={2} animate>TURN DIP 3 ON</RainbowText></Centre>
    <Panel x={58} y={74} w={204} h={76}>
      <div style={{ position: 'absolute', left: 10, top: 8, display: 'flex', gap: 8 }}>
        {[1, 2, 3, 4, 5, 6].map(n => { const on = n === 3, c = on ? hue(300) : '#7b7b7b'; return <div key={n} style={{ width: 24, display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 5 }}>
          <div style={{ width: 24, height: 46, border: `1px solid ${c}`, boxSizing: 'border-box', position: 'relative', background: on ? 'rgba(255,0,200,.15)' : 'transparent', boxShadow: on ? `0 0 8px ${c}` : 'none' }}><div style={{ position: 'absolute', left: 2, right: 2, height: 20, top: on ? 2 : 22, background: on ? `linear-gradient(${hue(300)}, ${hue(330)})` : c }}></div></div>
          <ScreenText color={on ? '#fff' : c} style={{ position: 'static' }}>{String(n)}</ScreenText></div>; })}
      </div>
    </Panel>
    <Panel x={22} y={156} w={276} h={30}>
      <ScreenText y={5} align="center" width={274}>The PN532 (NFC) is not answering.</ScreenText>
      <ScreenText y={17} align="center" width={274} color={hue(52)}>Only one of DIP 1-5 may be ON.</ScreenText>
    </Panel>
  </AppFrame>;

  return <AppFrame title="TAGS WRITTEN" footer="ESC: back" {...F} backdrop={<FreqScene sunX={270} sunR={28} horizon={204} sparkles={20} />}>
    <Panel x={10} y={50} w={300} h={146}>
      <TextArea top={8} lines={[{ t: 'Moonlight Yoga  [locked]', c: hue(300) }, { t: '  frequencylocal.com/n/moonlight-yoga', c: hue(180) }, { t: '  NTAG215  2026-10-20 16:12', c: LGREY }, '', { t: 'Riverside Coffee', c: hue(300) }, { t: '  frequencylocal.com/n/riverside', c: hue(180) }, { t: '  NTAG213  2026-10-19 11:40', c: LGREY }, '', { t: 'Log: /frequency/written.csv', c: hue(52) }]} />
    </Panel>
  </AppFrame>;
}
window.FrequencyApp = FrequencyApp;
