The animated unicorn boot screen, ported from `apps/boot/unicorn_boot.cpp` and extended with the blast-off finale (tremble, bank left, rocket over the sun, rainbow spray floods the screen); use for splash and "Waking the unicorn…" states, and cut to the next screen on `onDone`.

```jsx
<BootScene scale={2} />                 // with the blast-off finale (default)
<BootScene scale={2} finale={false} />  // plain loader: gallop + 137% bar
<BootScene tagline="Ready to sparkle" loop={false} onDone={() => setScreen('menu')} />
```

Always 320x240 internally; `scale` is an integer upscale with pixelated rendering.
